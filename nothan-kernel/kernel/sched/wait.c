/*
 * kernel/sched/wait.c - Wait queue primitives (sleep/wake)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/mm.h>		/* list helpers */
#include <asm/irqflags.h>

/**
 * wait_event - put the current task to sleep on a wait queue
 * @wq: the wait queue to sleep on
 *
 * Sets the current task state to TASK_UNINTERRUPTIBLE, links it into the wait
 * queue, and gives up the CPU.  The task must be woken by a matching wake_up().
 *
 * All three steps happen with interrupts masked, and schedule() returns still
 * masked (see its contract in core.c), so the sequence is atomic against
 * wake_up() running from an ISR.  It has to be: @rt.run_list is the same field
 * the runqueue links through, so an interrupt landing inside list_add_tail()
 * would corrupt both lists at once, and a wake_up() arriving after the state
 * was set but before the task actually slept would be lost entirely.
 */
void wait_event_locked(struct wait_queue_head *wq)
{
	struct task_struct *curr = runqueue.curr;

	curr->__state = TASK_UNINTERRUPTIBLE;

	/* Reuse rt.run_list to link into the wait queue. */
	list_add_tail(&curr->rt.run_list, &wq->task_list);

	schedule();	/* returns with the mask exactly as the caller left it */
}

void wait_event(struct wait_queue_head *wq)
{
	struct task_struct *curr = runqueue.curr;
	unsigned long flags;

	flags = local_irq_save();

	curr->__state = TASK_UNINTERRUPTIBLE;

	/* Reuse rt.run_list to link into the wait queue. */
	list_add_tail(&curr->rt.run_list, &wq->task_list);

	schedule();

	local_irq_restore(flags);
}

/**
 * wake_up - wake the first task waiting on a wait queue
 * @wq: the wait queue to wake
 *
 * Removes the first waiting task from the queue, sets its state back to
 * TASK_RUNNING, and enqueues it on the runqueue.  Callable from interrupt
 * context, so it saves and restores rather than forcing interrupts on.
 */
void wake_up(struct wait_queue_head *wq)
{
	unsigned long flags = local_irq_save();
	struct task_struct *p;

	if (list_empty(&wq->task_list)) {
		local_irq_restore(flags);
		return;
	}

	p = list_first_entry(&wq->task_list, struct task_struct, rt.run_list);

	list_del(&p->rt.run_list);
	p->__state = TASK_RUNNING;
	enqueue_task(&runqueue, p);

	local_irq_restore(flags);
}
