#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/mm.h>		/* list helpers */

/**
 * wait_event - put the current task to sleep on a wait queue
 * @wq: the wait queue to sleep on
 *
 * Sets the current task state to TASK_UNINTERRUPTIBLE, removes it from
 * the runqueue, and calls schedule().  The task must be woken by
 * a matching wake_up() call.
 */
void wait_event(struct wait_queue_head *wq)
{
	struct task_struct *curr = runqueue.curr;

	curr->__state = TASK_UNINTERRUPTIBLE;

	/* Reuse rt.run_list to link into the wait queue. */
	list_add_tail(&curr->rt.run_list, &wq->task_list);

	schedule();
}

/**
 * wake_up - wake the first task waiting on a wait queue
 * @wq: the wait queue to wake
 *
 * Removes the first waiting task from the queue, sets its state
 * back to TASK_RUNNING, and enqueues it on the runqueue.
 */
void wake_up(struct wait_queue_head *wq)
{
	if (list_empty(&wq->task_list))
		return;

	struct task_struct *p = list_first_entry(&wq->task_list, struct task_struct, rt.run_list);

	list_del(&p->rt.run_list);
	p->__state = TASK_RUNNING;
	enqueue_task(&runqueue, p);
}
