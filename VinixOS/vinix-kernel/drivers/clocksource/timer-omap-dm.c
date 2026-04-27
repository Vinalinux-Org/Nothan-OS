/* ============================================================
 * timer.c
 * ------------------------------------------------------------
 * AM335x DMTimer2 driver.
 * ============================================================ */

#include "types.h"
#include "timer.h"
#include "irq.h"
#include "scheduler.h"
#include "intc.h"
#include "mmio.h"
#include "uart.h"
#include "trace.h"

/* ============================================================
 * Clock Management Registers
 * ============================================================ */

#define CM_PER_BASE             0x44E00000
#define CM_PER_L4LS_CLKSTCTRL   (CM_PER_BASE + 0x00)
#define CM_PER_TIMER2_CLKCTRL   (CM_PER_BASE + 0x80)

/* CM_PER_L4LS_CLKSTCTRL bits */
#define CLKTRCTRL_MASK          0x3
#define CLKTRCTRL_SW_WKUP       0x2      /* Force wakeup */

/* CM_PER_TIMER2_CLKCTRL bits */
#define MODULEMODE_MASK         0x3
#define MODULEMODE_ENABLE       0x2      /* Module explicitly enabled */
#define IDLEST_MASK             0x3
#define IDLEST_SHIFT            16
#define IDLEST_FUNC             0x0      /* Fully functional */

/* ============================================================
 * DMTimer2 Register Offsets
 * ============================================================ */

#define TIDR            0x00    /* Identification register */
#define TIOCP_CFG       0x10    /* OCP interface configuration */
#define TISTAT          0x14    /* Status (reset) */
#define IRQSTATUS_RAW   0x24    /* IRQ raw status */
#define IRQSTATUS       0x28    /* IRQ status (after mask) */
#define IRQENABLE_SET   0x2C    /* IRQ enable set */
#define IRQENABLE_CLR   0x30    /* IRQ enable clear */
#define TCLR            0x38    /* Control register */
#define TCRR            0x3C    /* Counter register */
#define TLDR            0x40    /* Load register */
#define TTGR            0x44    /* Trigger register */
#define TWPS            0x48    /* Write-posted pending */
#define TMAR            0x4C    /* Match register */
#define TCAR1           0x50    /* Capture register 1 */
#define TSICR           0x54    /* Synchronous interface control */
#define TCAR2           0x58    /* Capture register 2 */

/* TIOCP_CFG bits */
#define TIOCP_SOFTRESET         (1 << 0)

/* TISTAT bits */
#define TISTAT_RESETDONE        (1 << 0)

/* TSICR bits */
#define TSICR_POSTED            (1 << 2)

/* TWPS bits */
#define TWPS_W_PEND_TCLR        (1 << 0)
#define TWPS_W_PEND_TCRR        (1 << 1)
#define TWPS_W_PEND_TLDR        (1 << 2)

/* TCLR bits */
#define TCLR_ST                 (1 << 0)    /* Start/stop timer */
#define TCLR_AR                 (1 << 1)    /* Auto-reload */

/* IRQSTATUS bits */
#define IRQ_MAT_IT_FLAG         (1 << 0)    /* Match interrupt */
#define IRQ_OVF_IT_FLAG         (1 << 1)    /* Overflow interrupt */
#define IRQ_TCAR_IT_FLAG        (1 << 2)    /* Capture interrupt */

/* ============================================================
 * Timer State
 * ============================================================ */

static volatile uint32_t timer_ticks = 0;

/* CRITICAL: must run before any timer register access. */
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

    mmio_write32(CM_PER_TIMER2_CLKCTRL, MODULEMODE_ENABLE);

    uint32_t timeout = 100000;
    while (timeout--) {
        val = mmio_read32(CM_PER_TIMER2_CLKCTRL);
        uint32_t idlest = (val >> IDLEST_SHIFT) & IDLEST_MASK;
        uint32_t modulemode = val & MODULEMODE_MASK;
        if (idlest == IDLEST_FUNC && modulemode == MODULEMODE_ENABLE) return;
    }

    pr_err("[TIMER] clock enable timeout (TIMER2_CLKCTRL=0x%08x)\n",
                mmio_read32(CM_PER_TIMER2_CLKCTRL));
    while (1);
}

/* INTC EOI runs in irq_dispatch() — handler only clears peripheral IRQ
 * and routes the tick through the registered clock_event_device. */
#include "vinix/clocksource.h"

static struct clock_event_device omap_dmtimer_clkevt;

static int omap_dmtimer_set_periodic(struct clock_event_device *dev)
{
    (void)dev;
    /* Hardware is configured for 10 ms periodic in timer_init();
     * nothing to do here yet. Stub for future runtime mode switch. */
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
    enable_irq(TIMER2_IRQ);

    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR);
    timeout = 10000;
    while ((mmio_read32(DMTIMER2_BASE + TWPS) & TWPS_W_PEND_TCLR) && timeout--);
    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_AR | TCLR_ST);

    /* Hand the tick over to the clockevents framework — IRQ
     * handler now dispatches via clkevt->event_handler instead
     * of calling scheduler_tick directly. */
    omap_dmtimer_clkevt.name                = "omap-dmtimer";
    omap_dmtimer_clkevt.rating              = 200;
    omap_dmtimer_clkevt.set_state_periodic  = omap_dmtimer_set_periodic;
    clockevents_register_device(&omap_dmtimer_clkevt);

    pr_info("[TIMER] DMTimer2 running (%u ms tick, irq %u)\n", period_ms, TIMER2_IRQ);
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

#define TIMER_FCLK_HZ   24000000
#define TICKS_PER_MS     (TIMER_FCLK_HZ / 1000)

/* Free-running setup for delay_ms() before timer_init() takes over. */
void timer_early_init(void)
{
    if ((mmio_read32(CM_PER_L4LS_CLKSTCTRL) & CLKTRCTRL_MASK) != CLKTRCTRL_SW_WKUP)
        mmio_write32(CM_PER_L4LS_CLKSTCTRL, CLKTRCTRL_SW_WKUP);

    #define CM_DPLL_BASE                0x44E00500
    #define CM_DPLL_CLKSEL_TIMER2_CLK  (CM_DPLL_BASE + 0x08)
    #define CLKSEL_CLK_M_OSC            0x1

    mmio_write32(CM_DPLL_CLKSEL_TIMER2_CLK, CLKSEL_CLK_M_OSC);
    while ((mmio_read32(CM_DPLL_CLKSEL_TIMER2_CLK) & 0x3) != CLKSEL_CLK_M_OSC)
        ;

    mmio_write32(CM_PER_TIMER2_CLKCTRL, MODULEMODE_ENABLE);
    while (((mmio_read32(CM_PER_TIMER2_CLKCTRL) >> IDLEST_SHIFT) & IDLEST_MASK) != IDLEST_FUNC)
        ;

    mmio_write32(DMTIMER2_BASE + TIOCP_CFG, TIOCP_SOFTRESET);
    while (mmio_read32(DMTIMER2_BASE + TIOCP_CFG) & TIOCP_SOFTRESET)
        ;

    mmio_write32(DMTIMER2_BASE + TCLR, TCLR_ST);
}

void delay_ms(uint32_t ms)
{
    uint32_t start = mmio_read32(DMTIMER2_BASE + TCRR);
    uint32_t ticks = ms * TICKS_PER_MS;

    while ((mmio_read32(DMTIMER2_BASE + TCRR) - start) < ticks)
        ;
}

/* ============================================================
 * Platform driver wiring
 * ============================================================ */

#include "platform_device.h"

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

#include "vinix/init.h"
module_platform_driver(omap_dmtimer_driver);