/*
 * kernel/sched/core.c - Core scheduler: runqueue, schedule(), and tick
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/printk.h>

struct rq runqueue;
int need_resched;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

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

	/* Pre-fill the switch frame (see fork.c for layout): */
	*--sp = (unsigned long)idle_main;	/* lr → PC */
	*--sp = 0;				/* r11 */
	*--sp = 0;				/* r10 */
	*--sp = 0;				/* r9  */
	*--sp = 0;				/* r8  */
	*--sp = 0;				/* r7  */
	*--sp = 0;				/* r6  */
	*--sp = (unsigned long)idle_main;	/* r5 (fallback exit) */
	*--sp = (unsigned long)idle_main;	/* r4 (fn) */

	idle_tsk.stack     = sp;
	idle_tsk.__state   = TASK_RUNNING;
	idle_tsk.exit_state = 0;
	idle_tsk.pid       = 0;
	idle_tsk.prio      = IDLE_PRIO;
	idle_tsk.rt.time_slice = RR_TIMESLICE;
	idle_tsk.rt.on_rq  = 0;

	const char *name = "idle";
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		idle_tsk.comm[i] = name[i];
	idle_tsk.comm[i] = '\0';

	enqueue_task(&runqueue, &idle_tsk);
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

	idle_task_init();

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

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	if (prev == next)
		return;

	if (prev)
		__switch_to(prev, next);
	else
		__asm__ __volatile__ (
			"ldr sp, [%0, #0]\n"
			"ldmfd sp!, {r4-r11, pc}\n"
			: : "r" (next));
}

void scheduler_tick(void)
{
	struct task_struct *curr = runqueue.curr;

	if (!curr)
		return;

	if (--curr->rt.time_slice > 0)
		return;

	curr->rt.time_slice = RR_TIMESLICE;
	need_resched = 1;
}
