#ifndef _WAIT_H
#define _WAIT_H
/*
 * Host stand-in for the wait queues.
 *
 * The harness never blocks: it drives the receive path directly and then looks
 * in the ring, so a wakeup has nowhere to go.  Counting them is still worth
 * something — a datagram accepted without a wake is an owner that sleeps
 * through its own mail, and that is a real bug this can catch.
 */
#include <nothan/types.h>

struct wait_queue_head { unsigned long wakes; };

static inline void init_waitqueue_head(struct wait_queue_head *wq) { wq->wakes = 0; }
static inline void wake_up(struct wait_queue_head *wq)             { wq->wakes++; }
static inline void wait_event_locked(struct wait_queue_head *wq)   { (void)wq; }

#define wait_event_cond(wq, cond)	do { (void)(cond); } while (0)
#endif
