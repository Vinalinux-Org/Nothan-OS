#ifndef _WAIT_H
#define _WAIT_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/sched.h>
#include <nothan/timer.h>
#include <asm/irqflags.h>

/**
 * struct wait_queue_head - queue of tasks waiting for an event
 * @task_list: linked list of sleeping tasks
 */
struct wait_queue_head {
	struct list_head task_list;
};

/*
 * Define a wait queue that is already valid at link time.
 *
 * Preferred over calling init_waitqueue_head() from an init function: an
 * uninitialised queue has next/prev NULL, and the first sleeper's
 * list_add_tail() writes straight through address 0.  That is a kernel-mode
 * NULL write in whatever task happened to block first — a long way from the
 * line that forgot the call.  Making it impossible to forget is cheaper than
 * making it easy to find.
 */
#define DEFINE_WAIT_QUEUE(name)						\
	struct wait_queue_head name = {					\
		.task_list = { &(name).task_list, &(name).task_list }	\
	}

static inline void init_waitqueue_head(struct wait_queue_head *wq)
{
	list_init(&wq->task_list);
}

/*
 * Sleep on @wq.  The caller must already hold the interrupt mask, and gets it
 * back on return — schedule() preserves it (see its contract in core.c).
 *
 * That is what makes wait_event_cond() below race-free.  Testing a condition
 * and then sleeping has to be one indivisible step: otherwise the event can
 * arrive in the gap, find nobody queued, and the sleeper never wakes.  With
 * the mask held across both, there is no gap.
 */
void wait_event_locked(struct wait_queue_head *wq);

/**
 * wait_event_cond() - sleep until @cond is true
 * @wq: queue the waker signals
 * @cond: expression re-evaluated after every wakeup
 *
 * A loop, not an if: waking does not prove the condition holds — another task
 * may have consumed whatever arrived before this one ran.
 */
#define wait_event_cond(wq, cond)						do {										unsigned long __wq_flags = local_irq_save();													while (!(cond))									wait_event_locked(wq);															local_irq_restore(__wq_flags);					} while (0)

void wait_event(struct wait_queue_head *wq);
void wake_up(struct wait_queue_head *wq);

/**
 * struct wait_timeout - the timer behind wait_event_cond_timeout()
 *
 * PROTECTION: every field is touched with interrupts masked, by the sleeper
 * setting it up and by the timer callback running from the tick.  The callback
 * has to unlink the task from the wait queue itself, which is why this exists
 * at all: @rt.run_list is the same field the runqueue links through, so a
 * waker that enqueues a task still linked into a wait queue corrupts both
 * lists at once.  msleep() gets away without it only because nothing it sleeps
 * on is a wait queue.
 */
struct wait_timeout {
	struct timer_list	timer;
	struct task_struct	*task;
	volatile int		fired;
};

void wait_timeout_arm(struct wait_timeout *wt, unsigned long ms);
void wait_timeout_disarm(struct wait_timeout *wt);

/**
 * wait_event_cond_timeout() - sleep until @cond, or until @ms have passed
 *
 * Evaluates to 1 if the condition held, 0 if the time ran out.
 *
 * For waiting on hardware.  A condition that depends on a device is a
 * condition that can simply never become true — a missed interrupt, a
 * peripheral left in the wrong state — and wait_event_cond() has no answer to
 * that but to hang.  The alternative in this tree until now was a busy loop
 * with a jiffy deadline, which does not hang but burns whatever priority the
 * caller happens to run at: on a 60 Hz panel a task waiting for a frame
 * boundary thirty times a second spins away a quarter of the machine.
 */
#define wait_event_cond_timeout(wq, cond, ms)				\
({									\
	struct wait_timeout __wt;					\
	unsigned long __wt_flags = local_irq_save();			\
									\
	wait_timeout_arm(&__wt, (ms));					\
	while (!(cond) && !__wt.fired)					\
		wait_event_locked(wq);					\
	wait_timeout_disarm(&__wt);					\
	local_irq_restore(__wt_flags);					\
	!!(cond);							\
})

#endif /* _WAIT_H */
