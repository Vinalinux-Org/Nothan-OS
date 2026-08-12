/*
 * drivers/clocksource/timer-ti-dm.c - TI DMTimer2 driver (10 ms scheduler tick)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/sched.h>
#include <nothan/timer.h>
#include <nothan/printk.h>
#include <nothan/platform.h>
#include <nothan/init.h>
#include <nothan/delay.h>

/*
 * DMTimer2 at PA 0x48040000 (L4_PER), VA 0xF0040000.
 * Clock: 24 MHz M_OSC crystal. IRQ: 68.
 *
 * Clock setup (must happen in order):
 *  1. CM_PER_L4LS_CLKSTCTRL -> SW_WKUP  (wake L4LS clock domain)
 *  2. CM_DPLL_CLKSEL_TIMER2  -> M_OSC   (select 24 MHz source)
 *  3. CM_PER_TIMER2_CLKCTRL  -> ENABLE  (enable module clock)
 *  4. Wait IDLEST = FUNCTIONAL
 */

/* DMTimer2 register base (VA) */
#define DMTIMER2_BASE		0xF0040000
#define DMTIMER2_IRQ		68

/* PRCM CM_PER domain (VA: PA 0x44E00000 -> 0xF0E00000) */
#define CM_PER_L4LS_CLKSTCTRL	0xF0E00000	/* PA 0x44E00000 */
#define CM_PER_TIMER2_CLKCTRL	0xF0E00080	/* PA 0x44E00080 */

/* PRCM CM_DPLL domain (VA: PA 0x44E00500 -> 0xF0E00500) */
#define CM_DPLL_CLKSEL_TIMER2	0xF0E00508	/* PA 0x44E00508 */

#define CLKTRCTRL_SW_WKUP	0x2
#define CLKSEL_M_OSC		0x1
#define MODULEMODE_ENABLE	0x2
#define IDLEST_FUNCTIONAL	(0x0 << 16)
#define IDLEST_MASK		(0x3 << 16)

/* DMTimer2 register offsets */
#define TIOCP_CFG		0x10
#define IRQSTATUS		0x28
#define IRQENABLE_SET		0x2C
#define TCLR			0x38
#define TCRR			0x3C
#define TLDR			0x40
#define TWPS			0x48
#define TSICR			0x54

#define TIOCP_SOFTRESET		(1 << 0)
#define TSICR_POSTED		(1 << 2)
#define TWPS_W_PEND_TCLR	(1 << 0)
#define TWPS_W_PEND_TCRR	(1 << 1)
#define TWPS_W_PEND_TLDR	(1 << 2)
#define TCLR_AR			(1 << 1)
#define TCLR_ST			(1 << 0)
#define IRQ_OVF_IT_FLAG		(1 << 1)

/*
 * PROTECTION: none needed.  Written only by the tick handler, read as a single
 * word elsewhere, and a 32-bit access is atomic on this core.  Readers may see
 * a value one tick stale, which is what a tick counter means anyway.
 */
static volatile unsigned long jiffies;

/* ------------------------------------------------------------------
 * DMTimer3 — free-running clocksource, no interrupt
 *
 * The 10 ms tick is far too coarse to measure anything this kernel
 * cares about (wakeup-to-run latency, cache behaviour, IRQ latency all
 * live well below one tick).  DMTimer3 runs untouched across the full
 * 32-bit range and is only ever read.
 *
 * Ref: DMTimer3 at PA 0x48042000 (TRM Ch02 memory map, Ch20 timers).
 *      L4_PER maps PA 0x48000000 -> VA 0xF0000000, so VA = 0xF0042000.
 *      CM_PER_TIMER3_CLKCTRL  = CM_PER  + 0x84   (TRM Ch08)
 *      CLKSEL_TIMER3_CLK      = CM_DPLL + 0x0C   (TRM Ch08, reset = M_OSC)
 * ------------------------------------------------------------------ */

#define DMTIMER3_BASE		0xF0042000
#define CM_PER_TIMER3_CLKCTRL	0xF0E00084
#define CM_DPLL_CLKSEL_TIMER3	0xF0E0050C

/* TSC_CYCLES_PER_US lives in nothan/timer.h — udelay() needs it too. */

/* False until tsc_probe() has the counter running; see clocksource_ready(). */
static volatile int tsc_running;

/*
 * The hardware counter is 32-bit and wraps every 2^32 / 24 MHz ~= 179 s.
 * Rather than reconstruct a 64-bit value from a "high half" (which would
 * jump backwards by 179 s during the window between a hardware wrap and
 * the next tick noticing it), accumulate elapsed cycles into @tsc_base on
 * every tick.  The delta is computed with unsigned 32-bit subtraction, so
 * it stays correct across a wrap as long as less than 179 s passes between
 * updates — the tick runs every 10 ms, so that margin is ~18000x.
 *
 * Reader/writer consistency uses a seqlock rather than masking interrupts:
 * timer_cycles() is meant to be callable from IRQ context and from the
 * latency measurements themselves, and disabling interrupts inside the
 * instrument would perturb the very thing being measured.
 */
static volatile u64 tsc_base;	/* cycles accumulated up to @tsc_last */
static volatile u32 tsc_last;	/* TCRR sampled at the last accumulate */
static volatile u32 tsc_seq;	/* even = stable, odd = update in flight */

#define tsc_barrier()	__asm__ __volatile__ ("" : : : "memory")

/* Called from the 10 ms tick.  Only writer of @tsc_base / @tsc_last. */
static void tsc_update(void)
{
	u32 now = mmio_read32(DMTIMER3_BASE + TCRR);

	tsc_seq++;
	tsc_barrier();

	tsc_base += (u32)(now - tsc_last);
	tsc_last = now;

	tsc_barrier();
	tsc_seq++;
}

/**
 * timer_cycles() - 24 MHz cycles elapsed since boot
 *
 * Returns a monotonic 64-bit cycle count.  Safe from any context.
 * Deliberately returns raw cycles, not microseconds: the kernel is
 * linked -nostdlib with no libgcc, so a 64-bit division would not link,
 * and conversion has no business sitting in a measurement hot path.
 */
u64 timer_cycles(void)
{
	u32 s1, s2, now, last;
	u64 base;

	do {
		s1 = tsc_seq;
		tsc_barrier();

		base = tsc_base;
		last = tsc_last;
		now = mmio_read32(DMTIMER3_BASE + TCRR);

		tsc_barrier();
		s2 = tsc_seq;
	} while ((s1 & 1u) || s1 != s2);

	return base + (u32)(now - last);
}

/**
 * cycles_to_us() - convert a 32-bit cycle delta to microseconds
 * @cycles: cycle count, must fit in 32 bits (i.e. under ~179 s)
 *
 * Division by the constant 24 compiles to a reciprocal multiply, so this
 * pulls in no libgcc helper.  Intervals longer than 179 s must be split
 * by the caller.
 */
u32 cycles_to_us(u32 cycles)
{
	return cycles / TSC_CYCLES_PER_US;
}

/** clocksource_ready() - true once DMTimer3 is actually counting */
int clocksource_ready(void)
{
	return tsc_running;
}

/*
 * tsc_probe - bring up DMTimer3 as a free-running counter
 *
 * Same PRCM bring-up as DMTimer2 (both live in the L4LS clock domain),
 * except that no interrupt is ever enabled and TLDR stays 0 so the
 * counter wraps across the full 32-bit range.
 */
static void tsc_probe(void)
{
	unsigned int timeout;
	u32 val;

	/* L4LS is shared with DMTimer2 and normally already awake; the check
	 * is idempotent so this does not depend on probe ordering. */
	if ((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) {
		mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
		timeout = 10000;
		while (((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) && timeout--)
			;
	}

	/* Reset value is already M_OSC, but say so explicitly. */
	mmio_write32(CM_DPLL_CLKSEL_TIMER3, CLKSEL_M_OSC);
	timeout = 10000;
	while (((mmio_read32(CM_DPLL_CLKSEL_TIMER3) & 0x3) != CLKSEL_M_OSC) && timeout--)
		;

	mmio_write32(CM_PER_TIMER3_CLKCTRL, MODULEMODE_ENABLE);
	timeout = 100000;
	while (timeout--) {
		val = mmio_read32(CM_PER_TIMER3_CLKCTRL);
		if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL && (val & 0x3) == MODULEMODE_ENABLE)
			break;
	}

	mmio_write32(DMTIMER3_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TIOCP_CFG) & TIOCP_SOFTRESET) && timeout--)
		;

	mmio_write32(DMTIMER3_BASE + TSICR, TSICR_POSTED);
	mmio_write32(DMTIMER3_BASE + TCLR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;

	/* No IRQ is ever enabled for this timer — just drop stale flags. */
	mmio_write32(DMTIMER3_BASE + IRQSTATUS, 0x7);

	mmio_write32(DMTIMER3_BASE + TLDR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TLDR) && timeout--)
		;

	mmio_write32(DMTIMER3_BASE + TCRR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCRR) && timeout--)
		;

	tsc_base = 0;
	tsc_last = 0;
	tsc_seq = 0;

	mmio_write32(DMTIMER3_BASE + TCLR, TCLR_AR | TCLR_ST);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;

	/* Last: from here on udelay() may busy-wait on this counter. */
	tsc_running = 1;
}

/*
 * tsc_selftest - prove the counter advances and the delay path terminates
 *
 * This used to double as a check on udelay()'s calibration, back when udelay
 * counted instructions: a result near 1000 us meant the core really was at
 * 1 GHz, and roughly double meant it was not.  That is how the undervolt and,
 * later, a cache-line alignment effect were both spotted.
 *
 * udelay() now waits on this very counter, so timing it here is circular and
 * the number will always come out near 1000.  What it still proves is worth
 * the two lines: the counter advances monotonically, the seqlock read returns,
 * and udelay()'s 64-bit arithmetic reaches its target instead of overflowing
 * or spinning forever.  A wrong TSC_CYCLES_PER_US would show up here too.
 */
static void tsc_selftest(void)
{
	u64 c0, c1;
	u32 elapsed;

	c0 = timer_cycles();
	udelay(1000);
	c1 = timer_cycles();

	elapsed = (u32)(c1 - c0);

	printk("[TSC] DMTimer3 @ 24 MHz free-running, 1 us = %u cycles\n",
	       TSC_CYCLES_PER_US);
	printk("[TSC] delay path: udelay(1000) measured %lu cycles = %lu us\n",
	       elapsed, cycles_to_us(elapsed));
}

static void timer_irq_handler(unsigned int irq)
{
	(void)irq;

	mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);
	tsc_update();
	jiffies++;
	scheduler_tick();
	run_local_timers();
}

/**
 * get_jiffies() - Return the current tick count
 *
 * Return: Number of timer ticks since boot (1 tick = 10 ms).
 */
unsigned long get_jiffies(void)
{
	return jiffies;
}

/*
 * timer_probe - Initialize DMTimer2 as a 10 ms scheduler tick
 *
 * Configures the PRCM to enable the timer clock from the 24 MHz M_OSC,
 * soft-resets the timer, sets up the auto-reload value for 10 ms
 * (240,000 cycles), and enables the overflow interrupt (IRQ 68) which
 * drives the preemptive scheduler tick.
 */
static int timer_probe(struct platform_device *pdev)
{
	(void)pdev;

	/* Free-running clocksource first: everything after this point can be
	 * measured, including the rest of this probe. */
	tsc_probe();
	tsc_selftest();

	/* Step 1: Force L4LS clock domain to SW_WKUP. */
	u32 val = mmio_read32(CM_PER_L4LS_CLKSTCTRL);
	unsigned int timeout;

	if ((val & 0x3) != CLKTRCTRL_SW_WKUP) {
		mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
		timeout = 10000;
		while (((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) && timeout--)
			;
	}

	/* Step 2: Select M_OSC (24 MHz) and wait for readback to confirm. */
	mmio_write32(CM_DPLL_CLKSEL_TIMER2, CLKSEL_M_OSC);
	while ((mmio_read32(CM_DPLL_CLKSEL_TIMER2) & 0x3) != CLKSEL_M_OSC)
		;

	/* Step 3: Enable module clock and wait until IDLEST = FUNCTIONAL. */
	mmio_write32(CM_PER_TIMER2_CLKCTRL, MODULEMODE_ENABLE);
	timeout = 100000;
	while (timeout--) {
		val = mmio_read32(CM_PER_TIMER2_CLKCTRL);
		if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL && (val & 0x3) == MODULEMODE_ENABLE)
			break;
	}

	/* Step 4: Soft-reset the timer and wait for reset done. */
	mmio_write32(DMTIMER2_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TIOCP_CFG) & TIOCP_SOFTRESET) && timeout--)
		;

	/* Step 5: Enable posted mode and stop timer before configuring. */
	mmio_write32(DMTIMER2_BASE + TSICR, TSICR_POSTED);
	mmio_write32(DMTIMER2_BASE + TCLR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;

	/* Clear any pending interrupts. */
	mmio_write32(DMTIMER2_BASE + IRQSTATUS, 0x7);

	/* Step 6: Configure reload value for 10 ms @ 24 MHz. */
	u32 reload = 0xFFFFFFFF - 240000 + 1;

	mmio_write32(DMTIMER2_BASE + TLDR, reload);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TLDR) && timeout--)
		;

	mmio_write32(DMTIMER2_BASE + TCRR, reload);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCRR) && timeout--)
		;

	/* Step 7: Enable IRQ and register handler. */
	mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, IRQ_OVF_IT_FLAG);
	request_irq(DMTIMER2_IRQ, timer_irq_handler);
	intc_enable_irq(DMTIMER2_IRQ);

	/* Step 8: Start the timer with auto-reload. */
	mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;
	/* Timer intentionally NOT started yet — timer_start() after sched_init() */
	printk("[TIMER] DMTimer2 @ 24 MHz, 10 ms tick, IRQ %d\n", DMTIMER2_IRQ);
	return 0;
}

/**
 * timer_start() - Start the DMTimer2 counter
 *
 * Called after sched_init() so the first tick does not preempt before
 * the scheduler is ready.
 */
void timer_start(void)
{
	unsigned int timeout = 10000;
	mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;
}

static struct platform_driver timer_driver = {
	.probe = timer_probe,
};

static int __init omap_timer_init(void)
{
	timer_driver.drv.name = "omap_timer";
	return platform_driver_register(&timer_driver);
}
device_initcall(omap_timer_init);
