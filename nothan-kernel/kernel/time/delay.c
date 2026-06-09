#include <nothan/delay.h>
#include <nothan/time.h>
#include <nothan/sched.h>
#include <nothan/timer.h>

/**
 * udelay() - busy-wait loop using ARM subs/bne
 * @usec: number of microseconds to delay
 *
 * Cortex-A8 @ 1 GHz: subs + bne ~ 2 cycles -> ~500 loops per us.
 */
#define LOOPS_PER_US	500

static void __delay(unsigned long loops)
{
	__asm__ __volatile__ (
		"1:	subs	%0, %0, #1\n"
		"	bne	1b\n"
		: "+r" (loops)
		:
		: "cc"
	);
}

void udelay(unsigned long usec)
{
	__delay(usec * LOOPS_PER_US);
}

/**
 * mdelay() - busy-wait loop for milliseconds
 * @msec: number of milliseconds to delay
 */
void mdelay(unsigned long msec)
{
	while (msec--)
		udelay(1000);
}

/* Called from timer IRQ context when msleep expires. */
static void msleep_callback(struct timer_list *t)
{
	struct task_struct *task = (struct task_struct *)t->data;

	if (task->__state != TASK_RUNNING) {
		task->__state = TASK_RUNNING;
		if (!task->rt.on_rq)
			enqueue_task(&runqueue, task);
	}
}

/**
 * msleep() - sleep for @msecs milliseconds (process context only)
 * @msecs: number of milliseconds to sleep
 *
 * Uses a kernel timer + schedule().  The IRQ-off section makes the
 * timer-arm + state-change + wait-list-add atomic with respect to
 * the tick ISR, closing the lost-wakeup race.
 */
void msleep(unsigned long msecs)
{
	unsigned long ticks = (msecs * HZ + 999) / 1000;
	if (ticks == 0)
		return;

	struct timer_list timer;

	init_timer(&timer);
	timer.expires = get_jiffies() + ticks;
	timer.function = msleep_callback;
	timer.data = (unsigned long)runqueue.curr;

	/*
	 * Critical section: arm timer and transition to sleep
	 * must be atomic vs. the timer IRQ.
	 */
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	add_timer(&timer);
	set_current_state(TASK_UNINTERRUPTIBLE);

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();		/* block until timer callback wakes us */

	del_timer(&timer);
}
