#ifndef _WAIT_H
#define _WAIT_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/sched.h>
#include <asm/irqflags.h>

/**
 * struct wait_queue_head - queue of tasks waiting for an event
 * @task_list: linked list of sleeping tasks (linked via task->rt.run_list)
 */
struct wait_queue_head {
	struct list_head task_list;
};

static inline void init_waitqueue_head(struct wait_queue_head *wq)
{
	list_init(&wq->task_list);
}

/*
 * Low-level helpers used by the wait_event() macro and by completion.c.
 * Both assume the caller holds IRQs masked (via local_irq_save()).
 */
void __prepare_to_wait(struct wait_queue_head *wq, unsigned int state);
void __finish_wait(void);

void wake_up(struct wait_queue_head *wq);	/* wake the first waiter */
void wake_up_all(struct wait_queue_head *wq);	/* wake every waiter   */

/**
 * wait_event - block the current task until @cond becomes true
 * @wqp:  pointer to the wait queue head to sleep on
 * @cond: condition expression, re-evaluated after every wakeup
 *
 * Race-free against wakers in IRQ context.  IRQs are masked across the
 * whole check-and-sleep sequence and __schedule() both runs and returns
 * with IRQs still masked, so a wake_up() can neither slip between the
 * condition test and the task being queued (lost wakeup) nor enqueue the
 * task twice (double enqueue).
 *
 * @cond must be an expression that a wake_up() path eventually makes true;
 * a spurious wakeup simply re-evaluates it and sleeps again.
 */
#define wait_event(wqp, cond)						\
	do {								\
		unsigned long __we_flags;				\
		local_irq_save(__we_flags);				\
		while (!(cond)) {					\
			__prepare_to_wait((wqp), TASK_UNINTERRUPTIBLE);	\
			__schedule();					\
		}							\
		__finish_wait();					\
		local_irq_restore(__we_flags);				\
	} while (0)

#endif /* _WAIT_H */
