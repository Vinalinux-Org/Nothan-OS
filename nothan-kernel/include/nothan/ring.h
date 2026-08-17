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
 * Two ways in and two ways out, for two sizes of element.  put/get copy the
 * whole element and are right for the small ones — a descriptor index, a
 * character.  reserve/commit and peek/release hand out a pointer to the slot
 * instead, so a large element is written once where it will live and read
 * where it lies.  For a 1500 byte datagram that is the difference between
 * moving it once and moving it three times, and it is also what lets DMA
 * eventually write straight into the slot: the ring stops caring who filled
 * it, which is a change of producer rather than a change of design.
 *
 * NOT safe for two producers or two consumers.  That is the whole contract,
 * and a second producer needs its own ring rather than a lock around this one.
 *
 * The producer never touches @tail, which rules out overwrite-the-oldest when
 * full: that would need the producer to advance the consumer's index, and the
 * absence of a second writer is the only reason there is no lock here.  A
 * consumer that wants the freshest data rather than the oldest can drain and
 * discard, because @tail is its own — the capability is kept, on the side that
 * already owns the index.
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
/* Producer side, in place.  Returns the slot to fill, or NULL if the	\
 * ring is full — a full ring is an error to handle, not a broken	\
 * assumption, so this reports rather than blocking or overwriting.	\
 * Which of those the caller wants differs by path: a packet may be	\
 * dropped and counted, a keystroke may not.				\
 *									\
 * The slot stays private until name##_commit(), so a producer that	\
 * reserves and then decides against it simply does not commit.  There	\
 * is no way to un-reserve because there is nothing to undo. */		\
static inline type *name##_reserve(struct name##_ring *r)		\
{									\
	unsigned int head = r->head;					\
									\
	if (head - r->tail >= (1u << (order)))				\
		return (type *)0;					\
									\
	return &r->buf[head & ((1u << (order)) - 1u)];			\
}									\
									\
/* Publish the reserved slot.  Only valid after a name##_reserve() that	\
 * returned non-NULL. */						\
static inline void name##_commit(struct name##_ring *r)			\
{									\
	/* The element must be visible before the index that publishes	\
	 * it.  Without this the consumer can read a slot that has not	\
	 * been written yet, and nothing faults. */			\
	dmb();								\
									\
	r->head = r->head + 1u;						\
}									\
									\
/* Consumer side, in place.  Returns the oldest element, or NULL when	\
 * empty.  The pointer stays valid until name##_release(): the producer	\
 * cannot reach this slot again before tail moves past it, which is	\
 * exactly what release does.						\
 *									\
 * This is the half that makes a large element worth having in a ring at	\
 * all.  Copying a 1500 byte datagram out only to have the reader copy	\
 * it somewhere else is two traversals of memory to move it once. */	\
static inline type *name##_peek(struct name##_ring *r)			\
{									\
	unsigned int tail = r->tail;					\
									\
	if (r->head == tail)						\
		return (type *)0;					\
									\
	/* Read the index before the element, so the element read cannot	\
	 * be hoisted above the emptiness test. */			\
	dmb();								\
									\
	return &r->buf[tail & ((1u << (order)) - 1u)];			\
}									\
									\
/* Give the peeked slot back to the producer.  Nothing may read through	\
 * the peeked pointer afterwards. */					\
static inline void name##_release(struct name##_ring *r)		\
{									\
	/* Release the slot only after it has been read, or the producer	\
	 * may refill it under us. */					\
	dmb();								\
									\
	r->tail = r->tail + 1u;						\
}									\
									\
/* Copy in.  The whole-element form, for elements small enough that a	\
 * copy is cheaper than tracking a borrow.  Returns 0 if full. */	\
static inline int name##_put(struct name##_ring *r, type v)		\
{									\
	type *slot = name##_reserve(r);					\
									\
	if (!slot)							\
		return 0;						\
									\
	*slot = v;							\
	name##_commit(r);						\
	return 1;							\
}									\
									\
/* Copy out.  Returns 0 when empty. */					\
static inline int name##_get(struct name##_ring *r, type *out)		\
{									\
	type *slot = name##_peek(r);					\
									\
	if (!slot)							\
		return 0;						\
									\
	*out = *slot;							\
	name##_release(r);						\
	return 1;							\
}									\
									\
struct name##_ring	/* force a semicolon at the use site */

#endif /* _NOTHAN_RING_H */
