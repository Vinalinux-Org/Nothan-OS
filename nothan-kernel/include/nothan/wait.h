#ifndef _WAIT_H
#define _WAIT_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/sched.h>
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

#endif /* _WAIT_H */
