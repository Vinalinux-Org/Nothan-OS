/*
 * kernel/time/delay.c - Busy-wait and sleep delay primitives
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/delay.h>
#include <nothan/time.h>
#include <nothan/sched.h>
#include <nothan/timer.h>
#include <asm/irqflags.h>

/*
 * Counting instructions is not a way to measure time.
 *
 * This used to be a calibrated loop: 500 iterations of subs/bne per
 * microsecond, on the assumption of a 1 GHz core and two cycles per
 * iteration.  Both halves of that were wrong in ways that only showed up once
 * there was a real clock to check against.
 *
 * The core is not always at 1 GHz — the bootloader stays at 600 MHz when the
 * PMIC does not confirm the higher rail — which alone makes every delay 1.67x
 * too long or, worse in a future configuration, too short.
 *
 * And the loop is two instructions, so whether it straddles an instruction
 * cache line depends on where the linker happens to place it.  Measured on
 * hardware, the same source came out at 24019 cycles with the loop inside one
 * line and 36026 with it across two — a 50% swing caused by editing an
 * unrelated file.  Every driver that waits on hardware with udelay() had its
 * timing quietly change with each rebuild, and a layout that ran *faster*
 * than the calibration assumed would have waited less than the datasheet
 * requires, which is the kind of fault that shows up as an occasional bad
 * register read rather than as an error.
 *
 * So wait on the clocksource instead: 24 MHz regardless of what the CPU is
 * doing, unaffected by code placement.  Granularity is one MMIO read of
 * DMTimer3, on the order of 0.1-0.2 us, so udelay(1) overshoots.  That is the
 * right direction to be wrong in — callers need "at least this long".
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

/**
 * udelay() - busy-wait for at least @usec microseconds
 * @usec: minimum delay in microseconds
 */
void udelay(unsigned long usec)
{
	u64 start, want;

	/*
	 * Before the clocksource is running, timer_cycles() keeps returning the
	 * same value and a wait on it would never finish.  Early boot gets the
	 * old calibrated loop — inaccurate, but the init sequences that run
	 * this early use generous margins, and there is nothing better to use.
	 */
	if (!clocksource_ready()) {
		__delay(usec * LOOPS_PER_US);
		return;
	}

	start = timer_cycles();
	want = (u64)usec * TSC_CYCLES_PER_US;

	while (timer_cycles() - start < want)
		;
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
	 * Arming the timer, marking ourselves asleep and giving up the CPU is
	 * one masked region.  Previously the mask was dropped just before
	 * schedule(), leaving exactly the window this section exists to close:
	 * the timer could expire and msleep_callback() could re-enqueue the
	 * task before it had actually stopped running.  schedule() returns
	 * still masked, so nothing here needs to re-enable in between.
	 */
	unsigned long flags = local_irq_save();

	add_timer(&timer);
	set_current_state(TASK_UNINTERRUPTIBLE);

	schedule();		/* block until timer callback wakes us */

	del_timer(&timer);

	local_irq_restore(flags);
}
