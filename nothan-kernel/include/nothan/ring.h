#ifndef _NOTHAN_RING_H
#define _NOTHAN_RING_H

/*
 * include/nothan/ring.h - the one hand-off from an interrupt to a task
 *
 * One producer, one consumer, fixed capacity, no locking at all.
 *
 * This is the hottest path the box will have: every received packet, every
 * block of audio samples, every character off a UART.  It is also the path
 * kernel-roadmap.md §9.2 constrains hardest — an interrupt handler is allowed
 * to recognise, queue and wake, nothing more — so what sits between the
 * handler and the task has to be cheap enough that "queue it" really is one
 * step.
 *
 * The reason there is no lock is structural, not an optimisation.  Each index
 * has exactly one writer: the producer advances @head and never touches @tail,
 * the consumer advances @tail and never touches @head.  Two writers to one
 * word is what a race is; with one writer each there is nothing to serialise,
 * and that property is visible by reading the code rather than by trusting
 * that every caller remembered to mask interrupts.  design-philosophy.md §1
 * asks for exactly that — bugs a log cannot chase have to be excluded by
 * construction, and the construction here is "one writer per index".
 *
 * Dropping the mask on this path is not a micro-optimisation either.  The one
 * measured region that masked interrupts across real work cost 10.8 ms
 * (os-architecture.md §3.6), and interrupt latency is the budget an audio
 * deadline is actually spent from.
 *
 * The barriers are the part that cannot be left to the caller.  ARMv7 may
 * reorder the write of an element past the write of @head, and a consumer that
 * saw the new head first would read a slot the producer had not filled — a
 * corruption with no fault and no log line.  So the barrier lives inside
 * ring_put(), one place, and no driver ever writes a ring by hand.
 *
 * Capacity is a power of two so the wrap is a mask.  The indices are free
 * running and are never reduced: the difference head - tail is the occupancy
 * and stays correct across the 32-bit wrap by unsigned arithmetic, which is
 * why one slot does not have to be sacrificed to tell full from empty.
 *
 * NOT safe for two producers or two consumers.  That is the whole contract,
 * and a second producer needs its own ring rather than a lock around this one.
 *
 * Two hand-rolled rings already exist in the tree — the UART receive buffer
 * and the console log — written before this header.  They are deliberately
 * left alone for now: both are working code that has just been through several
 * rounds of debugging, and rewriting them buys nothing today.  This exists so
 * the next one is not a third variant.
 */

#include <nothan/types.h>
#include <asm/barrier.h>

/**
 * DEFINE_RING() - declare a ring type and its operations
 * @name:  prefix for the generated type and functions
 * @type:  element type
 * @order: log2 of the capacity in elements
 *
 * Generates struct @name_ring plus @name_put/@name_get/@name_used, typed, so
 * an element is stored by assignment rather than by memcpy through a void *.
 * The type escapes into the signatures, which means a driver queueing the
 * wrong thing fails to compile instead of at three in the morning.
 */
#define DEFINE_RING(name, type, order)					\
									\
struct name##_ring {							\
	volatile unsigned int	head;	/* producer only */		\
	volatile unsigned int	tail;	/* consumer only */		\
	type			buf[1u << (order)];			\
};									\
									\
/* Elements waiting.  Safe from either side; may be stale the instant it	\
 * returns, which is inherent and not a defect: the producer can only	\
 * make it larger and the consumer only smaller, so each side's own	\
 * decision stays valid. */						\
static inline unsigned int name##_used(const struct name##_ring *r)	\
{									\
	return r->head - r->tail;					\
}									\
									\
/* Producer side.  Returns 0 if the ring is full — a full ring is an	\
 * error to handle, not a broken assumption, so this reports rather than	\
 * blocking or overwriting.  Which of those the caller wants differs by	\
 * path: a packet may be dropped and counted, a keystroke may not. */	\
static inline int name##_put(struct name##_ring *r, type v)		\
{									\
	unsigned int head = r->head;					\
									\
	if (head - r->tail >= (1u << (order)))				\
		return 0;						\
									\
	r->buf[head & ((1u << (order)) - 1u)] = v;			\
									\
	/* The element must be visible before the index that publishes	\
	 * it.  Without this the consumer can read a slot that has not	\
	 * been written yet, and nothing faults. */			\
	dmb();								\
									\
	r->head = head + 1u;						\
	return 1;							\
}									\
									\
/* Consumer side.  Returns 0 when empty. */				\
static inline int name##_get(struct name##_ring *r, type *out)		\
{									\
	unsigned int tail = r->tail;					\
									\
	if (r->head == tail)						\
		return 0;						\
									\
	/* Read the index before the element, so the element read cannot	\
	 * be hoisted above the emptiness test. */			\
	dmb();								\
									\
	*out = r->buf[tail & ((1u << (order)) - 1u)];			\
									\
	/* And release the slot only after it has been copied out, or	\
	 * the producer may refill it under us. */			\
	dmb();								\
									\
	r->tail = tail + 1u;						\
	return 1;							\
}									\
									\
struct name##_ring	/* force a semicolon at the use site */

#endif /* _NOTHAN_RING_H */
