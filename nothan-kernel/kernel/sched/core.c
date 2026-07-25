/*
 * kernel/sched/core.c - Core scheduler: runqueue, schedule(), and tick
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/printk.h>
#include <asm/irqflags.h>

struct rq runqueue;
int need_resched;
bool sched_running = false;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

/*
 * Deferred-free list. A task that calls do_exit() is still running on its own
 * kernel stack, so it cannot free that stack itself. do_exit() queues the
 * dying task here; the next task scheduled in frees it.
 */
static struct list_head dead_list;

void sched_defer_free(struct task_struct *tsk)
{
	list_add(&tsk->rt.run_list, &dead_list);
	pr_debug("[DEAD] queued pid=%d kstack=%p\n",
		 tsk->pid, tsk->kstack_base);
}

static void reap_dead(void)
{
	struct sched_rt_entity *rt, *tmp;

	list_for_each_entry_safe(rt, tmp, &dead_list,
				 struct sched_rt_entity, run_list) {
		struct task_struct *z = container_of(rt, struct task_struct, rt);

		if (z == runqueue.curr)
			continue;	/* never free the stack we're running on */
		pr_debug("[REAP] put pid=%d kstack=%p\n", z->pid, z->kstack_base);
		list_del(&rt->run_list);	/* off dead_list */
		task_unregister(z);		/* off all_tasks + pid_hash */
		/*
		 * Drop the existence ref. Frees now if nobody else holds one; if a
		 * task_find() caller (e.g. sys_kill) still holds a ref, the free is
		 * deferred until they put() — closing the UAF.
		 */
		put_task_struct(z);
	}
}

/*
 * Memory-bound task registry. Every task (kernel thread, user task, idle) is
 * linked on the global all_tasks list at creation and hashed by pid, removed
 * at reap. No fixed cap — the only ceiling is RAM (creation fails earlier at
 * kmalloc). Finds tasks blocked off the runqueue too (needed by kill).
 *
 * Single-core: local_irq_save/restore is the mutual exclusion (same as before).
 */
LIST_HEAD(all_tasks);	/* every live task; walked by sys_gettasklist */
static struct list_head pid_hash_table[PID_HASH_SIZE];

static inline struct list_head *pid_bucket(int pid)
{
	return &pid_hash_table[(unsigned)pid % PID_HASH_SIZE];
}

void task_register(struct task_struct *p)
{
	unsigned long flags;

	local_irq_save(flags);
	list_add(&p->tasks, &all_tasks);
	list_add(&p->pid_hash, pid_bucket(p->pid));
	local_irq_restore(flags);
}

void task_unregister(struct task_struct *p)
{
	unsigned long flags;

	local_irq_save(flags);
	list_del(&p->tasks);
	list_del(&p->pid_hash);
	local_irq_restore(flags);
}

struct task_struct *task_find(int pid)
{
	unsigned long flags;
	struct task_struct *found = NULL;
	struct task_struct *p;

	local_irq_save(flags);
	list_for_each_entry(p, pid_bucket(pid), struct task_struct, pid_hash) {
		if (p->pid == pid) {
			found = p;
			found->refcount++;	/* hand caller a ref (find+get atomic under
						 * this mask; reap can't free it meanwhile) */
			break;
		}
	}
	local_irq_restore(flags);
	return found;
}

/*
 * Refcount lifetime.
 *
 * refcount = 1 at creation (existence ref). task_find() takes an extra ref for
 * its caller. reap_dead() drops the existence ref via put; the task_struct +
 * kernel stack are freed only when refcount hits 0. So a task task_find()'d by
 * sys_kill survives until that caller put()s, even if it exits meanwhile —
 * closing the UAF.
 *
 * Impl = plain int under local_irq_save/restore (single-core mutual exclusion).
 * On SMP, swap these two bodies for atomics; callers don't change.
 */
void get_task_struct(struct task_struct *p)
{
	unsigned long flags;

	local_irq_save(flags);
	p->refcount++;
	local_irq_restore(flags);
}

void put_task_struct(struct task_struct *p)
{
	unsigned long flags;
	int dead;

	local_irq_save(flags);
	dead = (--p->refcount == 0);
	local_irq_restore(flags);

	/*
	 * Freed outside the mask: once refcount is 0 the task is already off the
	 * registry (reap unregisters before put) and off every list, so no other
	 * context can reach p — this is the exclusive last reference.
	 */
	if (dead) {
		if (p->kstack_base)
			kfree(p->kstack_base);
		kfree(p);
	}
}

/* Idle task — always runnable, lowest priority, no kmalloc needed. */
#define IDLE_STACK_WORDS 256
static unsigned long idle_stack[IDLE_STACK_WORDS];
static struct task_struct idle_tsk;

static void idle_main(void)
{
	while (1) {
		__asm__ __volatile__ ("cpsie i\nwfi" : : : "memory");
		schedule();
	}
}

static void idle_task_init(void)
{
	unsigned long *sp = idle_stack + IDLE_STACK_WORDS;

	/* Pre-fill the switch frame (see spawn.c for layout): */
	*--sp = (unsigned long)idle_main;	/* lr → PC */
	*--sp = 0;				/* r11 */
	*--sp = 0;				/* r10 */
	*--sp = 0;				/* r9  */
	*--sp = 0;				/* r8  */
	*--sp = 0;				/* r7  */
	*--sp = 0;				/* r6  */
	*--sp = (unsigned long)idle_main;	/* r5 (fallback exit) */
	*--sp = (unsigned long)idle_main;	/* r4 (fn) */

		idle_tsk.stack      = sp;
		idle_tsk.user_sp    = 0;
		idle_tsk.user_lr    = 0;
		idle_tsk.__state    = TASK_RUNNING;
		idle_tsk.flags      = 0;
		idle_tsk.pid        = 0;
		idle_tsk.prio       = IDLE_PRIO;
		idle_tsk.rt.time_slice = RR_TIMESLICE;
		idle_tsk.rt.on_rq   = 0;
		idle_tsk.exit_code  = 0;
		idle_tsk.mm         = NULL;
		idle_tsk.refcount   = 1;	/* never exits/reaped → never put → never freed */
		list_init(&idle_tsk.wait_node);	/* idle never blocks; empty node */

	const char *name = "idle";
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		idle_tsk.comm[i] = name[i];
	idle_tsk.comm[i] = '\0';

	enqueue_task(&runqueue, &idle_tsk);
		runqueue.curr = &idle_tsk;

	task_register(&idle_tsk);
}

/**
 * sched_init() - Initialize the scheduler runqueue
 */
void sched_init(void)
{
	struct rq *rq = &runqueue;

	rq->active.bitmap = 0;
	rq->nr_running = 0;
	rq->curr = NULL;

	for (unsigned int i = 0; i < MAX_PRIO; i++)
		list_init(&rq->active.queue[i]);

	need_resched = 0;

	list_init(&dead_list);

	for (unsigned int i = 0; i < PID_HASH_SIZE; i++)
		list_init(&pid_hash_table[i]);

	idle_task_init();

	/*
	 * Bootstrap: set curr to idle_tsk so that any code running between
	 * sched_init() and the first schedule() sees a valid current task.
	 * Without this, timer ISR → scheduler_tick() → runqueue.curr->field
	 * dereferences NULL and causes a data abort.
	 */
	rq->curr = &idle_tsk;

	printk("[SCHED] %d prio levels, RR timeslice=%d tick(s), idle at %d\n",
	       MAX_PRIO, RR_TIMESLICE, IDLE_PRIO);
}

/**
 * __schedule() - Core reschedule: pick the next task and switch to it
 *
 * PRECONDITION:  IRQs are already masked by the caller.
 * POSTCONDITION: IRQs are still masked on return; the caller restores them.
 *
 * Keeping IRQs masked across the whole body lets a blocking primitive
 * (wait_event, wait_for_completion) commit the "add self to wait queue +
 * set state + schedule" sequence atomically, closing the lost-wakeup and
 * double-enqueue races.  IRQs must also stay masked across __switch_to so a
 * timer IRQ cannot fire mid-stack-switch.
 *
 * IRQs are re-enabled by the caller:
 *  - schedule() wrapper: local_irq_restore() after __schedule() returns
 *  - task_entry, for newly-created tasks (cpsie i at entry)
 *  - vector_irq path: cpsid i after the bl schedule (kept as-is)
 *
 * The idle task is always on the runqueue, so pick_next_task() never
 * returns NULL under normal operation.
 */
void __schedule(void)
{
	reap_dead();

	struct task_struct *prev = runqueue.curr;

	if (prev && prev->__state == TASK_RUNNING)
		enqueue_task(&runqueue, prev);

	struct task_struct *next = pick_next_task(&runqueue);
	if (!next) {
		/* Should never happen — idle task is always available. */
		runqueue.curr = NULL;
		return;
	}

	runqueue.curr = next;
	need_resched = 0;

	if (prev == next)
		return;

	sched_running = true;

	if (prev) {
		/* Set up user mapping BEFORE context switch — __switch_to to a
		 * new user task branches to user_task_trampoline directly and
		 * never returns, so mmu_switch_mm after it would be skipped. */
		if (next->mm)
			mmu_switch_mm(next->mm);
		__switch_to(prev, next);
	} else {
		if (next->mm)
			mmu_switch_mm(next->mm);
		__asm__ __volatile__ (
			"cps #0x1f\n"
			"ldr sp, [%0, #4]\n"
			"ldr lr, [%0, #8]\n"
			"cps #0x13\n"
			"ldr sp, [%0, #0]\n"
			"ldmfd sp!, {r4-r11, pc}\n"
			: : "r" (next));
	}

	/* Reached only when prev is resumed by a later __switch_to; IRQs
	 * are still masked (the resuming task switched us in under mask). */
}

/**
 * schedule() - Reschedule wrapper for ordinary callers
 *
 * Masks IRQs, runs the core reschedule, then restores the caller's prior
 * IRQ state.  Blocking primitives that already hold IRQs masked call
 * __schedule() directly instead.
 */
void schedule(void)
{
	unsigned long flags;

	local_irq_save(flags);
	__schedule();
	local_irq_restore(flags);
}

/*
 * Preemptive scheduling: the timer tick rotates the running task once its
 * RR timeslice is spent. (Was toggled to 0 during a 2026-06 A/B test; the
 * project has since chosen real preemptive multitasking so background tasks
 * can run alongside the GUI without it having to yield() cooperatively.)
 */
#define SCHED_PREEMPT  1

void scheduler_tick(void)
{
	struct task_struct *curr = runqueue.curr;

	if (!curr)
		return;

	if (--curr->rt.time_slice > 0)
		return;

	curr->rt.time_slice = RR_TIMESLICE;
#if SCHED_PREEMPT
	need_resched = 1;
#endif
}
