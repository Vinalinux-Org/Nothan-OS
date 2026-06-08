#include <nothan/types.h>
#include <nothan/sched.h>

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
