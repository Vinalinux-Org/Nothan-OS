/*
 * kernel/sched/wait.c - Wait queue primitives (sleep/wake)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * The blocking side lives in the wait_event() macro (see wait.h): it holds
 * IRQs masked across "test condition -> queue self -> __schedule()", which
 * is what makes the sleep race-free against a wake_up() firing from IRQ
 * context.  The helpers below carry out the individual steps.
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/mm.h>		/* list helpers */
#include <asm/irqflags.h>

/**
 * __prepare_to_wait - mark the current task blocked and queue it
 * @wq:    wait queue to sleep on
 * @state: TASK_UNINTERRUPTIBLE (interruptible variant lands with Layer 2)
 *
 * Caller must hold IRQs masked.  Reuses rt.run_list to link into the wait
 * queue; the task is off the runqueue while it sleeps (schedule() only
 * re-enqueues a task whose state is still TASK_RUNNING).
 */
void __prepare_to_wait(struct wait_queue_head *wq, unsigned int state)
{
	struct task_struct *curr = runqueue.curr;

	curr->__state = state;
	list_add_tail(&curr->rt.run_list, &wq->task_list);
}

/**
 * __finish_wait - return the current task to the running state
 *
 * By the time wait_event()'s loop exits, the task is no longer on the wait
 * queue: either the condition was already true on entry (never queued) or a
 * wake_up() dequeued it before making it runnable.  So only the state has to
 * be restored.
 */
void __finish_wait(void)
{
	runqueue.curr->__state = TASK_RUNNING;
}

/* Move the first waiter off @wq onto the runqueue. Caller holds IRQs masked. */
static void __wake_one(struct wait_queue_head *wq)
{
	struct task_struct *p = list_first_entry(&wq->task_list,
					struct task_struct, rt.run_list);

	list_del(&p->rt.run_list);
	p->__state = TASK_RUNNING;
	enqueue_task(&runqueue, p);
}

/**
 * wake_up - wake the first task waiting on @wq
 *
 * Safe to call from task or IRQ context: masking is saved/restored, so an
 * IRQ-context caller (already masked) is not disturbed.
 */
void wake_up(struct wait_queue_head *wq)
{
	unsigned long flags;

	local_irq_save(flags);
	if (!list_empty(&wq->task_list))
		__wake_one(wq);
	local_irq_restore(flags);
}

/**
 * wake_up_all - wake every task waiting on @wq
 */
void wake_up_all(struct wait_queue_head *wq)
{
	unsigned long flags;

	local_irq_save(flags);
	while (!list_empty(&wq->task_list))
		__wake_one(wq);
	local_irq_restore(flags);
}
