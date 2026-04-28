/* ============================================================
 * intc.c
 * ------------------------------------------------------------
 * AM335x INTC interrupt-controller driver.
 * ============================================================ */

#include "intc.h"
#include "mmio.h"
#include "cpu.h"
#include "platform_device.h"
#include "uart.h"
#include "vinix/irqchip.h"
#include "vinix/init.h"

void intc_init(void)
{
    mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);

    mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}

uint32_t intc_get_active_irq(void)
{
    uint32_t sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);

    if (sir & SPURIOUSIRQ_MASK) {
        return 128;
    }

    return sir & ACTIVEIRQ_MASK;
}

void intc_eoi(void)
{
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
    dsb();
}

void intc_enable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }

    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;

    mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

void intc_disable_irq(uint32_t irq_num)
{
    if (irq_num >= 128) {
        return;
    }

    uint32_t bank = irq_num / 32;
    uint32_t bit = irq_num % 32;

    mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}

void intc_set_priority(uint32_t irq_num, uint32_t priority)
{
    if (irq_num >= 128 || priority > 63) {
        return;
    }

    /* Routes to IRQ (not FIQ). */
    uint32_t ilr = (priority & 0x3F);
    mmio_write32(INTC_BASE + INTC_ILR(irq_num), ilr);
}

/* irq_chip vtable — kernel's enable_irq/disable_irq dispatch
 * here once irqchip_register has run. ack/eoi go through the
 * existing intc_eoi path; intc_get_active_irq feeds the IRQ
 * dispatcher. */
static void omap_intc_irq_mask(struct irq_data *d)
{
    intc_disable_irq(d->irq);
}

static void omap_intc_irq_unmask(struct irq_data *d)
{
    intc_enable_irq(d->irq);
}

static void omap_intc_irq_eoi(struct irq_data *d)
{
    (void)d;
    intc_eoi();
}

static struct irq_chip omap_intc_chip = {
    .name       = "omap-intc",
    .irq_mask   = omap_intc_irq_mask,
    .irq_unmask = omap_intc_irq_unmask,
    .irq_eoi    = omap_intc_irq_eoi,
};

static int omap_intc_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    pr_info("[INTC] probing %s @ 0x%08x\n",
                pdev->name, mem ? mem->start : 0);
    intc_init();
    irqchip_register(&omap_intc_chip);
    return 0;
}

static struct platform_driver omap_intc_driver = {
    .drv   = { .name = "omap-intc" },
    .probe = omap_intc_probe,
};

static int __init omap_intc_driver_init(void)
{
    return platform_driver_register(&omap_intc_driver);
}
subsys_initcall(omap_intc_driver_init);