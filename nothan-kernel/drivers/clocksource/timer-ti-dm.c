#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/sched.h>
#include <nothan/printk.h>

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

static volatile unsigned long jiffies;

static void timer_irq_handler(unsigned int irq)
{
	(void)irq;

	mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);
	jiffies++;
	scheduler_tick();
}

unsigned long get_jiffies(void)
{
	return jiffies;
}

/**
 * timer_init - Initialize DMTimer2 as a 10 ms scheduler tick
 *
 * Configures the PRCM to enable the timer clock from the 24 MHz M_OSC,
 * soft-resets the timer, sets up the auto-reload value for 10 ms
 * (240,000 cycles), and enables the overflow interrupt (IRQ 68) which
 * drives the preemptive scheduler tick.
 */
void timer_init(void)
{
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
	mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);
}
