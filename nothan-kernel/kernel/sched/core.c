#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/printk.h>

struct rq runqueue;
int need_resched;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

/**
 * sched_init() - Initialize the scheduler runqueue
 *
 * Clears the global runqueue, sets bitmap to zero, and initialises
 * each per-priority linked list head.
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

	printk("sched: init OK\n");
}

/**
 * schedule() - Pick the next task and switch to it
 *
 * The runqueue manipulation (enqueue prev, dequeue next) is a critical
 * section: a timer IRQ between those two calls would see on_rq=1 and
 * attempt list_move_tail(), corrupting the list.  Disable IRQs for
 * the duration of the runqueue update.
 */
void schedule(void)
{
	/* --- critical section: protect runqueue lists --- */
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	struct task_struct *prev = runqueue.curr;

	if (prev && prev->__state == TASK_RUNNING)
		enqueue_task(&runqueue, prev);

	struct task_struct *next = pick_next_task(&runqueue);
	if (!next) {
		runqueue.curr = NULL;
		__asm__ __volatile__ ("cpsie i" : : : "memory");
		return;
	}

	runqueue.curr = next;
	need_resched = 0;

	__asm__ __volatile__ ("cpsie i" : : : "memory");
	/* --- end critical section --- */

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

/**
 * scheduler_tick() - Called from the timer ISR
 *
 * Decrements the current task's timeslice.  Does NOT touch the
 * runqueue lists — that is schedule()'s job.  Just signals that a
 * reschedule is needed so the next schedule() call will rotate tasks.
 */
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
