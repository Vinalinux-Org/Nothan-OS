/*
 * kernel/ringtest.c - acceptance test for the lock-free ISR-to-task ring
 *
 * The claim in nothan/ring.h is that one producer and one consumer need no
 * lock, because each index has a single writer.  That is an argument, and an
 * argument about memory ordering on a machine with a store buffer is exactly
 * the kind that reads as obviously correct while being wrong — which is the
 * cell of the design-philosophy.md §1 table where a log is no help at all.
 *
 * So it is run in the shape it will actually be used in: the producer is the
 * timer interrupt, the consumer is an ordinary BG task, and nothing masks
 * anything.  Every real user will be this — a packet arriving, a block of
 * samples, a character — and none of them can be tested by calling put() and
 * get() from the same thread, which is what a self-contained unit test would
 * amount to here.
 *
 * The answer is known in advance and is not statistical: the producer only
 * advances its sequence number when a put succeeds, so the consumer must see
 * every number exactly once, in order, with no gaps.  A gap, a repeat or a
 * value out of order is a failure and panics naming both what was expected and
 * what arrived.  A ring that is momentarily full is not a failure — that is an
 * ordinary condition the interface reports — and it is counted separately so a
 * clean run can still be distinguished from one where nothing was contended.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/ring.h>
#include <nothan/sched.h>
#include <nothan/delay.h>
#include <nothan/printk.h>
#include <nothan/panic.h>

#if CONFIG_RING_TEST

/*
 * Four entries, against one producer per 1 ms tick and a consumer that sleeps
 * five.  Both numbers are chosen to make the ring fill.
 *
 * The first version used 64 entries and a 1 ms sleep, and passed 2000 values
 * without the ring ever filling once — which the run reported, because the
 * test says so rather than leaving it to be assumed.  That left `return 0` in
 * rt_put() unexecuted, and it is the only branch in the whole structure with a
 * decision in it: everything else is arithmetic that either works for every
 * input or none.  An untested branch in the one place a choice is made is
 * where a bug would live.
 *
 * Five produced per four consumed means the ring is full at the end of every
 * cycle, permanently, from the first second onwards.  Ordering is unaffected —
 * the producer only advances its sequence on a successful put — so the
 * consumer can still demand an unbroken run while the full path is hammered.
 */
#define RT_SLEEP_MS	5

DEFINE_RING(rt, u32, 2);

static struct rt_ring		rt_ring;
static u32			rt_seq;		/* producer: next value to send */
static unsigned long		rt_full;	/* producer: ring was full */
static unsigned long		rt_got;		/* consumer: values accepted */

#define RT_TARGET	2000u

/*
 * Producer — runs in the timer interrupt, from scheduler_tick().
 *
 * The sequence advances only on a successful put, so a full ring costs a
 * delay and not a hole.  That is what lets the consumer demand an unbroken
 * sequence rather than "a sequence with some allowance for loss", and a check
 * with no allowance in it is the only kind that can fail loudly.
 */
void ringtest_produce(void)
{
	if (rt_put(&rt_ring, rt_seq))
		rt_seq++;
	else
		rt_full++;
}

static void ringtest_consume(void)
{
	u32 expect = 0;

	printk("[RING] consumer up; expecting %u values strictly in order,"
	       " and the ring to fill (4 slots, 1 in per ms, drained every"
	       " %u ms)\n", RT_TARGET, RT_SLEEP_MS);

	while (rt_got < RT_TARGET) {
		u32 v;

		if (!rt_get(&rt_ring, &v)) {
			/*
			 * Empty: sleep, and sleep longer than the producer's
			 * period on purpose.  Spinning would drain the ring the
			 * instant anything landed and the full path would never
			 * run; sleeping one tick would very nearly keep up,
			 * which is what the first version did and why it
			 * reported the full path untested.
			 */
			msleep(RT_SLEEP_MS);
			continue;
		}

		if (v != expect)
			panic("ring broke ordering: expected %lu, got %lu,"
			      " after %lu values (%lu full events)",
			      (unsigned long)expect, (unsigned long)v,
			      rt_got, rt_full);

		expect++;
		rt_got++;

		if ((rt_got % 500u) == 0)
			printk("[RING] %lu/%u in order, %lu full events\n",
			       rt_got, RT_TARGET, rt_full);
	}

	printk("[RING] PASS: %lu values, none lost, none repeated,"
	       " none out of order; ring was full %lu times\n",
	       rt_got, rt_full);

	if (!rt_full)
		printk("[RING] NOTE: the ring never filled, so the full path"
		       " went untested this run\n");
}

#endif /* CONFIG_RING_TEST */

/**
 * ringtest_start() - launch the ring consumer
 *
 * Called from kernel_main with interrupts already masked, alongside the other
 * initial tasks.  The producer needs no starting: it is the tick.
 */
void ringtest_start(void)
{
#if CONFIG_RING_TEST
	struct task_struct *t = task_create(ringtest_consume, PRIO_BG,
					    "ring-rx");

	if (t)
		enqueue_task(&runqueue, t);
	else
		printk("[RING] could not create the consumer task\n");
#endif
}
