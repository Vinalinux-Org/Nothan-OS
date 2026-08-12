/*
 * kernel/sched/core.c - Core scheduler: runqueue, schedule(), and tick
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <asm/irqflags.h>
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/printk.h>

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
	printk("[DEAD] queued pid=%d kstack=%p\n",
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
		printk("[REAP] free pid=%d kstack=%p\n", z->pid, z->kstack_base);
		list_del(&rt->run_list);
		if (z->kstack_base)
			kfree(z->kstack_base);
		kfree(z);
	}
}

/* Idle task — always runnable, lowest priority, no kmalloc needed. */
#define IDLE_STACK_WORDS 256
static unsigned long idle_stack[IDLE_STACK_WORDS];
static struct task_struct idle_tsk;

/*
 * The idle task owns the global interrupt state outright — nobody is waiting
 * on a critical section it holds — so the unconditional forms are the right
 * ones here.  WFI needs interrupts enabled to be woken by one; schedule()
 * needs them masked.
 */
static void idle_main(void)
{
	while (1) {
		__asm__ __volatile__ ("cpsie i\nwfi" : : : "memory");
		local_irq_disable();
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
		idle_tsk.pid        = 0;
		idle_tsk.prio       = IDLE_PRIO;
		idle_tsk.rt.time_slice = RR_TIMESLICE;
		idle_tsk.rt.on_rq   = 0;
		idle_tsk.exit_code  = 0;
		idle_tsk.mm         = NULL;

	const char *name = "idle";
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		idle_tsk.comm[i] = name[i];
	idle_tsk.comm[i] = '\0';

	enqueue_task(&runqueue, &idle_tsk);
		runqueue.curr = &idle_tsk;
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
 * schedule() - Pick the next task and switch to it
 *
 * CONTRACT: enter with interrupts masked, return with interrupts masked.
 *
 * schedule() never changes the interrupt mask.  That is what lets a caller
 * make "decide to sleep, record it, give up the CPU" one atomic step:
 *
 *	flags = local_irq_save();
 *	set_current_state(TASK_UNINTERRUPTIBLE);
 *	list_add_tail(&curr->rt.run_list, &wq->task_list);
 *	schedule();
 *	local_irq_restore(flags);
 *
 * The previous version masked on entry and unmasked on every exit, which
 * meant a caller could not hold a critical section across it: the window
 * between marking a task as sleeping and actually sleeping was open to the
 * tick and to any wake_up() from an ISR, and both touch the same rt.run_list
 * the caller was mid-way through linking.
 *
 * A task resumed later by another __switch_to() returns from here with
 * interrupts still masked, exactly as it left them, and its own caller
 * restores its own saved flags.  Freshly created tasks do not come back
 * through here at all — they enter at task_entry, which enables interrupts
 * itself.
 *
 * The idle task is always on the runqueue, so pick_next_task() never returns
 * NULL.
 */
void schedule(void)
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

	/*
	 * Interrupts stay masked across __switch_to: a timer IRQ landing
	 * between "ldr sp, [next]" and "ldmfd ... pc" inside __switch_to would
	 * corrupt the task stack mid-switch.  Under the contract above the
	 * caller already masked them, so there is nothing to do here — which
	 * is the point.  Whoever masked is the one who unmasks.
	 */
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

	/*
	 * Reached only when prev is resumed by a later __switch_to, with
	 * interrupts masked exactly as they were when it gave up the CPU.
	 * Its own caller restores its own flags.
	 */
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
