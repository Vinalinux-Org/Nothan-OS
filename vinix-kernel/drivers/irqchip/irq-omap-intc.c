/*
 * AM335x Interrupt Controller (INTC) Driver
 *
 * Manages the top-level interrupt controller for the ARM Cortex-A8 core.
 * Handles enabling, disabling, prioritizing, and acknowledging hardware IRQs.
 */

#include "intc.h"
#include "mmio.h"
#include "cpu.h"
#include "platform_device.h"
#include "uart.h"
#include "vinix/irqchip.h"
#include "vinix/init.h"

/*
 * intc_init - Initialize the Interrupt Controller
 *
 * Masks all interrupts across all 4 banks (128 IRQs total) and
 * resets the controller's threshold priority and control registers.
 */
void intc_init(void)
{
    mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
    mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);

    mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);
    mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}

/*
 * intc_get_active_irq - Retrieve the highest priority pending IRQ
 *
 * Reads the Source IRQ (SIR) register. If a spurious interrupt is
 * detected, returns an invalid IRQ number (128) to ignore it.
 */
uint32_t intc_get_active_irq(void)
{
    uint32_t sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);

    if (sir & SPURIOUSIRQ_MASK) {
        return 128;
    }

    return sir & ACTIVEIRQ_MASK;
}

/*
 * intc_eoi - End of Interrupt acknowledgment
 *
 * Signals to the INTC that the current interrupt processing is complete,
 * allowing the controller to evaluate and assert new pending interrupts.
 */
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

/*
 * irq_chip vtable — integrates the AM335x INTC with the generic IRQ core
 *
 * Maps the generic enable_irq/disable_irq calls to the hardware-specific
 * intc_enable_irq/intc_disable_irq functions. The EOI path uses intc_eoi.
 */
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
arch_initcall(omap_intc_driver_init);