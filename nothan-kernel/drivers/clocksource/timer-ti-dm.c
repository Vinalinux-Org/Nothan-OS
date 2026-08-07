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
#include <nothan/time.h>
#include <nothan/panic.h>
#include <asm/irqflags.h>

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

/*
 * DMTimer3 — free-running clocksource for sched_clock()/vruntime (P0).
 * All addresses verified from reference/am335x/ (Ch.2 memory map, Ch.8 PRCM):
 *   DMTimer3 regs   PA 0x48042000 -> VA 0xF0042000 (in the L4_PER 32 MB map)
 *   TIMER3 CLKCTRL  PA 0x44E00084 -> VA 0xF0E00084 (CM_PER + 0x84)
 *   CLKSEL_TIMER3   PA 0x44E0050C -> VA 0xF0E0050C (CM_DPLL + 0x0C)
 * Register offsets are identical to DMTimer2 (same peripheral).
 */
#define DMTIMER3_BASE		0xF0042000
#define CM_PER_TIMER3_CLKCTRL	0xF0E00084
#define CM_DPLL_CLKSEL_TIMER3	0xF0E0050C

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

static volatile unsigned long jiffies;

static void timer_irq_handler(unsigned int irq)
{
	(void)irq;

	mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);
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
 * timer3_clocksource_init() - DMTimer3 as a free-running 24 MHz counter.
 *
 * Same PRCM bring-up as the DMTimer2 tick, but configured to free-run with NO
 * interrupt: reload 0 (full 32-bit range) + auto-reload + start. sched_clock()
 * then just reads TCRR. Called from timer_probe() so it is up before the
 * scheduler ever accounts vruntime.
 */
static void timer3_clocksource_init(void)
{
	unsigned int timeout;
	u32 val;

	/* L4LS clock domain (shared with DMTimer2) — ensure it is awake. */
	if ((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) {
		mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
		timeout = 10000;
		while (((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & 0x3) != CLKTRCTRL_SW_WKUP) && timeout--)
			;
	}

	/* Select M_OSC (24 MHz) and confirm. */
	mmio_write32(CM_DPLL_CLKSEL_TIMER3, CLKSEL_M_OSC);
	while ((mmio_read32(CM_DPLL_CLKSEL_TIMER3) & 0x3) != CLKSEL_M_OSC)
		;

	/* Enable the module clock, wait until functional. */
	mmio_write32(CM_PER_TIMER3_CLKCTRL, MODULEMODE_ENABLE);
	timeout = 100000;
	while (timeout--) {
		val = mmio_read32(CM_PER_TIMER3_CLKCTRL);
		if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL && (val & 0x3) == MODULEMODE_ENABLE)
			break;
	}

	/* Soft reset. */
	mmio_write32(DMTIMER3_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TIOCP_CFG) & TIOCP_SOFTRESET) && timeout--)
		;

	/* Posted mode; stop before configuring. */
	mmio_write32(DMTIMER3_BASE + TSICR, TSICR_POSTED);
	mmio_write32(DMTIMER3_BASE + TCLR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;

	/* Free-run: reload 0 (wrap 0xFFFFFFFF -> 0), start at 0, auto-reload, NO IRQ. */
	mmio_write32(DMTIMER3_BASE + TLDR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TLDR) && timeout--)
		;
	mmio_write32(DMTIMER3_BASE + TCRR, 0);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCRR) && timeout--)
		;
	mmio_write32(DMTIMER3_BASE + TCLR, TCLR_AR | TCLR_ST);
	timeout = 10000;
	while ((mmio_read32(DMTIMER3_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;

	printk("[TIMER] DMTimer3 @ 24 MHz free-running clocksource (sched_clock)\n");
}

/*
 * sched_clock() - monotonic nanoseconds for vruntime accounting.
 *
 * Reads the 24 MHz DMTimer3 counter (~41.7 ns resolution). The 32-bit counter
 * wraps every ~179 s; a software high word extends it to 64 bits. Safe against
 * the tick ISR via IRQ masking (single core). 1 cycle = 1000/24 ns.
 */
u64 sched_clock(void)
{
	static u32 last;
	static u64 cyc_hi;
	unsigned long flags;
	u32 now;
	u64 cycles;

	local_irq_save(flags);
	now = mmio_read32(DMTIMER3_BASE + TCRR);
	if (now < last)
		cyc_hi += 0x100000000ULL;	/* 32-bit counter wrapped */
	last = now;
	cycles = cyc_hi + now;
	local_irq_restore(flags);

	return cycles * 1000ULL / 24ULL;	/* 24 MHz cycles -> ns */
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

	/* Step 7: Enable IRQ and register handler.
	 *
	 * Failing here is not survivable and must not be papered over: with no
	 * tick there is no preemption, no jiffies and no msleep, so the kernel
	 * would boot, run the first task, and hang the instant anything waited
	 * for time to pass — a silent stop with no clue as to why. panic() names
	 * the cause while the UART still works. */
	mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, IRQ_OVF_IT_FLAG);
	if (request_irq(DMTIMER2_IRQ, timer_irq_handler, "dmtimer2-tick"))
		panic("timer: cannot claim IRQ %d - no scheduler tick",
		      DMTIMER2_IRQ);
	intc_enable_irq(DMTIMER2_IRQ);

	/* Step 8: Start the timer with auto-reload. */
	mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR);
	timeout = 10000;
	while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--)
		;
	/* Timer intentionally NOT started yet — timer_start() after sched_init() */
	printk("[TIMER] DMTimer2 @ 24 MHz, 10 ms tick, IRQ %d\n", DMTIMER2_IRQ);

	/* Bring up the free-running clocksource that feeds sched_clock()/vruntime. */
	timer3_clocksource_init();
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
