/*
 * kernel/sched/wait.c - Wait queue primitives (sleep/wake)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/mm.h>		/* list helpers */
#include <nothan/timer.h>
#include <nothan/time.h>
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

/*
 * Wake the sleeper whose time ran out.
 *
 * Runs from the tick, so the mask is already held by the interrupt path, but
 * take it anyway: this is called through the timer list and the contract there
 * does not promise it.
 *
 * The unlink is the whole reason this is not msleep's callback.  A task
 * sleeping on a wait queue is linked into it through rt.run_list, and that is
 * the same field the runqueue uses — enqueueing it without removing it first
 * splices the two lists together.  TASK_RUNNING is the test for "already
 * woken", set under the mask by wake_up(), so checking it here cannot race.
 */
static void wait_timeout_fn(struct timer_list *t)
{
	struct wait_timeout *wt = (struct wait_timeout *)t->data;
	unsigned long flags = local_irq_save();

	wt->fired = 1;

	if (wt->task->__state != TASK_RUNNING) {
		list_del(&wt->task->rt.run_list);
		wt->task->__state = TASK_RUNNING;
		if (!wt->task->rt.on_rq)
			enqueue_task(&runqueue, wt->task);
	}

	local_irq_restore(flags);
}

void wait_timeout_arm(struct wait_timeout *wt, unsigned long ms)
{
	unsigned long ticks = (ms * HZ + 999) / 1000;

	if (!ticks)
		ticks = 1;

	wt->task  = runqueue.curr;
	wt->fired = 0;

	init_timer(&wt->timer);
	wt->timer.expires  = get_jiffies() + ticks;
	wt->timer.function = wait_timeout_fn;
	wt->timer.data     = (unsigned long)wt;

	add_timer(&wt->timer);
}

void wait_timeout_disarm(struct wait_timeout *wt)
{
	del_timer(&wt->timer);
}
