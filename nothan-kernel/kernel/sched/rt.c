/*
 * kernel/sched/rt.c - O(1) bitmap-based real-time runqueue operations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/config.h>
#include <nothan/timer.h>
#include <nothan/printk.h>

/**
 * enqueue_task - add a task to the runqueue
 * @rq: the global runqueue
 * @p: task to enqueue
 *
 * Inserts the task at the tail of its priority queue and sets the
 * corresponding bitmap bit.  O(1).
 */
void enqueue_task(struct rq *rq, struct task_struct *p)
{
	if (p->rt.on_rq)
		return;
	list_add_tail(&p->rt.run_list, &rq->active.queue[p->prio]);
	sched_set_bit(rq, p->prio);
	rq->nr_running++;
	p->rt.on_rq = 1;

	check_preempt_curr(rq, p);
}

/**
 * check_preempt_curr() - a newly runnable task may be more urgent than the
 *                        one holding the CPU
 * @rq: the runqueue
 * @p: the task that just became runnable
 *
 * Lives inside enqueue_task() rather than at each wake site.  Every path that
 * makes a task runnable goes through there — wake_up(), complete(), the timer
 * callback, task creation — so none of them can forget, and a path added later
 * inherits it.  Putting the test at the call sites instead would be exactly the
 * "remember to do it" discipline design-philosophy.md §1 rules out.
 *
 * The three conditions are explicit rather than relying on the order of lines
 * elsewhere.  schedule() also calls enqueue_task(), to put the outgoing task
 * back; at that moment rq->curr is still that task, so "p != rq->curr" is what
 * keeps a reschedule from asking for another reschedule forever.  rq->curr is
 * NULL only during sched_init(), before there is anything to preempt.
 */
void check_preempt_curr(struct rq *rq, struct task_struct *p)
{
	if (p == rq->curr || !rq->curr)
		return;

#if CONFIG_SCHED_LATENCY
	/*
	 * Stamp the moment it became runnable — but only for a task that has
	 * run before.
	 *
	 * enqueue_task() is also how a brand new task joins the runqueue, and
	 * the first version counted that as a wakeup.  It produced 2 ms for the
	 * shell and 84 ms for storage_daemon, which are real intervals but
	 * measure "how long after being created did it first get the CPU" —
	 * time spent spawning other tasks and printing, nothing to do with
	 * waking.  Worse, those two outliers became the maximum, and since only
	 * a new maximum is reported, every genuine wakeup afterwards was
	 * measured and never shown.
	 */
	if (p->rt.ran_once)
		p->rt.wake_ts = timer_cycles();
#endif

	if (p->prio < rq->curr->prio)
		need_resched = 1;
}

/**
 * dequeue_task - remove a task from the runqueue
 * @rq: the global runqueue
 * @p: task to dequeue
 *
 * Removes the task from its priority queue.  Clears the bitmap bit
 * if the queue becomes empty.  O(1).
 */
void dequeue_task(struct rq *rq, struct task_struct *p)
{
	list_del(&p->rt.run_list);
	if (list_empty(&rq->active.queue[p->prio]))
		sched_clear_bit(rq, p->prio);
	rq->nr_running--;
	p->rt.on_rq = 0;
}

/**
 * pick_next_task - select and dequeue the highest-priority runnable task
 * @rq: the global runqueue
 *
 * Uses the bitmap to find the highest occupied priority level,
 * removes the first task from that queue, and returns it.
 * Returns NULL if no task is runnable.  O(1).
 *
 * Return: pointer to the next task, or NULL.
 */
struct task_struct *pick_next_task(struct rq *rq)
{
	if (!rq->nr_running)
		return NULL;

	int idx = sched_find_first_bit(rq->active.bitmap);
	if (idx >= MAX_PRIO)
		return NULL;

	struct task_struct *p = list_first_entry(&rq->active.queue[idx],
			     struct task_struct, rt.run_list);

	list_del(&p->rt.run_list);
	if (list_empty(&rq->active.queue[idx]))
		sched_clear_bit(rq, idx);
	rq->nr_running--;
	p->rt.on_rq = 0;

	return p;
}
