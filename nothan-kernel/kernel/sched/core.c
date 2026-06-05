#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/printk.h>

struct rq runqueue;
int need_resched;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

/**
 * sched_init - initialise the scheduler runqueue
 *
 * Clears the global runqueue, bitmap set to zero, and initialises
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
 * schedule - pick the next task and switch to it
 *
 * If the current task is still runnable, re-enqueue it before
 * picking the next.  Called from the timer tick (via need_resched)
 * or directly from yield / exit.
 */
void schedule(void)
{
	struct task_struct *prev = runqueue.curr;
	/* Current task is still runnable: put it back. */
	if (prev && prev->__state == TASK_RUNNING)
		enqueue_task(&runqueue, prev);

	struct task_struct *next = pick_next_task(&runqueue);
	if (!next) {
		runqueue.curr = NULL;
		return;
	}

	runqueue.curr = next;

	if (prev != next)
		__switch_to(prev, next);
}

/**
 * scheduler_tick - called from the timer ISR (every 10 ms)
 *
 * Decrements the current task's timeslice.  On expiry, resets the
 * timeslice, rotates the task to the tail of its priority queue
 * (round-robin), and sets need_resched so the next IRQ return
 * triggers a schedule().
 */
void scheduler_tick(void)
{
	struct task_struct *curr = runqueue.curr;

	if (!curr)
		return;

	if (--curr->rt.time_slice > 0)
		return;

	curr->rt.time_slice = RR_TIMESLICE;
	list_move_tail(&curr->rt.run_list, &runqueue.active.queue[curr->prio]);
	need_resched = 1;
}
