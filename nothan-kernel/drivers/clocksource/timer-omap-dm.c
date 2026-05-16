/*
 * drivers/clocksource/timer-omap-dm.c
 *
 * AM335x DMTimer2 — 10 ms scheduler tick, sourced from 24 MHz M_OSC crystal.
 */

#include "types.h"
#include "timer.h"
#include "irq.h"
#include "scheduler.h"
#include "intc.h"
#include "mmio.h"
#include "uart.h"
#include "trace.h"
#include "mach/prcm.h"
#include "nothan/clocksource.h"
#include "platform_device.h"
#include "nothan/init.h"

/* CM_PER_L4LS_CLKSTCTRL bits */
#define CLKTRCTRL_MASK          0x3
#define CLKTRCTRL_SW_WKUP       0x2

/* CM_DPLL_CLKSEL_TIMER2_CLK source selection */
#define CLKSEL_CLK_M_OSC        0x1

#define TIDR            0x00
#define TIOCP_CFG       0x10
#define TISTAT          0x14
#define IRQSTATUS_RAW   0x24
#define IRQSTATUS       0x28
#define IRQENABLE_SET   0x2C
#define IRQENABLE_CLR   0x30
#define TCLR            0x38
#define TCRR            0x3C
#define TLDR            0x40
#define TTGR            0x44
#define TWPS            0x48
#define TMAR            0x4C
#define TCAR1           0x50
#define TSICR           0x54
#define TCAR2           0x58

#define TIOCP_SOFTRESET         (1 << 0)
#define TISTAT_RESETDONE        (1 << 0)
#define TSICR_POSTED            (1 << 2)
#define TWPS_W_PEND_TCLR        (1 << 0)
#define TWPS_W_PEND_TCRR        (1 << 1)
#define TWPS_W_PEND_TLDR        (1 << 2)
#define TCLR_ST                 (1 << 0)    /* Start/stop */
#define TCLR_AR                 (1 << 1)    /* Auto-reload */
#define IRQ_MAT_IT_FLAG         (1 << 0)
#define IRQ_OVF_IT_FLAG         (1 << 1)    /* Overflow — used for tick */
#define IRQ_TCAR_IT_FLAG        (1 << 2)

static volatile uint32_t timer_ticks = 0;

/* The module clock must be enabled before any DMTimer2 register access;
 * accessing an unclocked peripheral causes a data abort. */
static void timer2_clock_enable(void)
{
    uint32_t val;

    val = mmio_read32(CM_PER_L4LS_CLKSTCTRL);
    if ((val & CLKTRCTRL_MASK) != CLKTRCTRL_SW_WKUP) {
        mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);
        uint32_t timeout = 10000;
        while (((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & CLKTRCTRL_MASK) != CLKTRCTRL_SW_WKUP) && timeout--);
        if (!timeout) { pr_err("[TIMER] L4LS wakeup timeout\n"); while (1); }
    }

    /* Source DMTimer2 from the 24 MHz crystal so the period math below
     * (and delay_ms()) match TIMER_FCLK_HZ. Reset default is not M_OSC,
     * so omitting this leaves the timer ticking at ~32 kHz. */
    mmio_write32(CM_DPLL_CLKSEL_TIMER2_CLK, CLKSEL_CLK_M_OSC);
    while ((mmio_read32(CM_DPLL_CLKSEL_TIMER2_CLK) & 0x3) != CLKSEL_CLK_M_OSC)
        ;

    mmio_write32(CM_PER_TIMER2_CLKCTRL, MODULEMODE_ENABLE);

    uint32_t timeout = 100000;
    while (timeout--) {
        val = mmio_read32(CM_PER_TIMER2_CLKCTRL);
        if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL &&
            (val & MODULEMODE_MASK) == MODULEMODE_ENABLE)
            return;
    }

    pr_err("[TIMER] clock enable timeout (TIMER2_CLKCTRL=0x%08x)\n",
                mmio_read32(CM_PER_TIMER2_CLKCTRL));
    while (1);
}

/* INTC EOI runs in irq_dispatch() — handler only clears peripheral IRQ
 * and routes the tick through the registered clock_event_device. */

static struct clock_event_device omap_dmtimer_clkevt;

static int omap_dmtimer_set_periodic(struct clock_event_device *dev)
{
    (void)dev;
    return 0;
}

static void timer_irq_handler(void *data)
{
    (void)data;
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, IRQ_OVF_IT_FLAG);

    timer_ticks++;

    if (omap_dmtimer_clkevt.event_handler)
        omap_dmtimer_clkevt.event_handler(&omap_dmtimer_clkevt);
}

void timer_init(void)
{
    timer2_clock_enable();

    mmio_write32(DMTIMER2_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
    uint32_t timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TIOCP_CFG) & TIOCP_SOFTRESET) && timeout--);
    if (!timeout) { pr_err("[TIMER] soft reset timeout\n"); while (1); }

    mmio_write32(DMTIMER2_BASE + TSICR, TSICR_POSTED);
    mmio_write32(DMTIMER2_BASE + TCLR, 0);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--);
    mmio_write32(DMTIMER2_BASE + IRQSTATUS, 0x7);

    /* 10 ms @ 24 MHz: reload = 0xFFFFFFFF − (24M/1000)·10 + 1. */
    uint32_t freq = 24000000;
    uint32_t period_ms = 10;
    uint32_t count = (freq / 1000) * period_ms;
    uint32_t reload = 0xFFFFFFFF - count + 1;
    mmio_write32(DMTIMER2_BASE + TLDR, reload);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TLDR) && timeout--);
    mmio_write32(DMTIMER2_BASE + TCRR, reload);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCRR) && timeout--);

    mmio_write32(DMTIMER2_BASE + IRQENABLE_SET, IRQ_OVF_IT_FLAG);
    if (request_irq(TIMER2_IRQ, timer_irq_handler, 0, "omap-dmtimer", NULL) != 0) {
        pr_err("[TIMER] request_irq failed\n");
        return;
    }

    /* Register clockevent BEFORE enable_irq + start: a tick firing in the
     * gap would find event_handler == NULL and silently drop. */
    omap_dmtimer_clkevt.name                = "omap-dmtimer";
    omap_dmtimer_clkevt.rating              = 200;
    omap_dmtimer_clkevt.set_state_periodic  = omap_dmtimer_set_periodic;
    clockevents_register_device(&omap_dmtimer_clkevt);

    enable_irq(TIMER2_IRQ);

    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--);
    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);

    pr_info("[TIMER] DMTimer2 running (%u ms tick, irq %u)\n", period_ms, TIMER2_IRQ);
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

#define TIMER_FCLK_HZ   24000000
#define TICKS_PER_MS     (TIMER_FCLK_HZ / 1000)

void delay_ms(uint32_t ms)
{
    uint32_t start = mmio_read32(DMTIMER2_BASE + TCRR);
    uint32_t ticks = ms * TICKS_PER_MS;

    while ((mmio_read32(DMTIMER2_BASE + TCRR) - start) < ticks)
        ;
}

static int omap_dmtimer_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    pr_info("[TIMER] probing %s @ 0x%08x irq %d\n",
                pdev->name, mem ? mem->start : 0, irq);
    timer_init();
    return 0;
}

static struct platform_driver omap_dmtimer_driver = {
    .drv   = { .name = "omap-dmtimer" },
    .probe = omap_dmtimer_probe,
};

module_platform_driver(omap_dmtimer_driver);