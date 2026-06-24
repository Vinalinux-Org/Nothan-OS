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

struct rq runqueue;
int need_resched;
bool sched_running = false;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

/*
 * Zombie reaping. A task that calls do_exit() is still running on its own
 * kernel stack, so it cannot free that stack itself. do_exit() queues the
 * dying task here; the next task scheduled in reaps it — freeing the kernel
 * stack and the task_struct (both kmalloc'd).
 */
static struct list_head zombie_list;

void sched_queue_zombie(struct task_struct *tsk)
{
	/* tsk is the running (off-runqueue) task, so its rt.run_list is free. */
	list_add(&tsk->rt.run_list, &zombie_list);
}

static void reap_zombies(void)
{
	struct sched_rt_entity *rt, *tmp;

	list_for_each_entry_safe(rt, tmp, &zombie_list,
				 struct sched_rt_entity, run_list) {
		struct task_struct *z = container_of(rt, struct task_struct, rt);

		if (z == runqueue.curr)
			continue;	/* never free the stack we're running on */
		list_del(&rt->run_list);
		unsigned long f0 = get_zone()->free_pages;	/* MEMCHK */
		if (z->kstack_base)
			kfree(z->kstack_base);
		/* MEMCHK: kstack should return +4 pages (16 KB). +0 = the leak. */
		printk("[MEMCHK] reap pid=%d: kstack free %lu->%lu pages (+%lu)\n",
		       z->pid, f0, get_zone()->free_pages,
		       get_zone()->free_pages - f0);
		kfree(z);
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
		idle_tsk.exit_state = 0;
		idle_tsk.pid        = 0;
		idle_tsk.tgid       = 0;
		idle_tsk.prio       = IDLE_PRIO;
		idle_tsk.rt.time_slice = RR_TIMESLICE;
		idle_tsk.rt.on_rq   = 0;
		idle_tsk.exit_code  = 0;
		idle_tsk.real_parent = &idle_tsk;
		idle_tsk.parent     = &idle_tsk;
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

	list_init(&zombie_list);

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
 * The idle task is always on the runqueue, so pick_next_task()
 * never returns NULL.
 */
void schedule(void)
{
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	/* Free any task that exited while running on its own kernel stack.
	 * Safe here: we run on the caller's stack, never the zombie's. */
	reap_zombies();

	struct task_struct *prev = runqueue.curr;

	if (prev && prev->__state == TASK_RUNNING)
		enqueue_task(&runqueue, prev);

	struct task_struct *next = pick_next_task(&runqueue);
	if (!next) {
		/* Should never happen — idle task is always available. */
		runqueue.curr = NULL;
		__asm__ __volatile__ ("cpsie i" : : : "memory");
		return;
	}

	runqueue.curr = next;
	need_resched = 0;

	/*
	 * Keep IRQs disabled across __switch_to to prevent a timer IRQ
	 * from firing between "ldr sp, [next]" and "ldmfd ... pc" inside
	 * __switch_to, which would corrupt the task stack mid-switch.
	 *
	 * IRQs are re-enabled:
	 *  - Here, after __switch_to returns (prev resumed after re-schedule)
	 *  - In task_entry, for newly-created tasks (cpsie i at entry)
	 *  - In vector_irq path: cpsid after schedule + rfefd restores CPSR
	 */
	if (prev == next) {
		__asm__ __volatile__ ("cpsie i" : : : "memory");
		return;
	}

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

	/* Reached only when prev is resumed by a later __switch_to. */
	__asm__ __volatile__ ("cpsie i" : : : "memory");
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
