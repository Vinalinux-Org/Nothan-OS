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
	if (list_empty(&curr->wait_node))	/* not already queued (loop re-entry) */
		list_add_tail(&curr->wait_node, &wq->task_list);
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
	struct task_struct *curr = runqueue.curr;

	curr->__state = TASK_RUNNING;
	/* Remove our own wait_node if a waker (or wake_up_task) hasn't already. */
	if (!list_empty(&curr->wait_node))
		list_del_init(&curr->wait_node);
}

/* Move the first waiter off @wq onto the runqueue. Caller holds IRQs masked. */
static void __wake_one(struct wait_queue_head *wq)
{
	struct task_struct *p = list_first_entry(&wq->task_list,
					struct task_struct, wait_node);

	list_del_init(&p->wait_node);
	p->__state = TASK_RUNNING;
	place_entity(&runqueue, p, 0);		/* clamp the woken task's vruntime */
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

/**
 * wake_up_task - force a specific task runnable, wherever it is blocked
 *
 * Used by sys_kill to reach a task blocked off the runqueue in a killable
 * (TASK_INTERRUPTIBLE) sleep. Removes it from its wait queue via the dedicated
 * wait_node (no stale-pointer hazard — that node is never the runqueue node)
 * and enqueues it. Once scheduled it hits its kill check and exits.
 *
 * A task in an uninterruptible sleep is left alone (not killable). A msleep()
 * timer sleeper is on no wait queue (wait_node empty), so this just enqueues it.
 */
void wake_up_task(struct task_struct *t)
{
	unsigned long flags;

	local_irq_save(flags);
	if (t->__state == TASK_INTERRUPTIBLE) {
		if (!list_empty(&t->wait_node))
			list_del_init(&t->wait_node);
		t->__state = TASK_RUNNING;
		if (!t->rt.on_rq) {
			place_entity(&runqueue, t, 0);	/* clamp on force-wake */
			enqueue_task(&runqueue, t);
		}
	}
	local_irq_restore(flags);
}
