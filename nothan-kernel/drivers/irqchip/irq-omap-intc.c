/*
 * drivers/irqchip/irq-omap-intc.c - OMAP INTC interrupt controller
 *
 * omap_intc_init() is called directly from kernel_main() before
 * do_initcalls(), mirroring Linux's init_IRQ() in start_kernel().
 * This guarantees INTC is ready before any driver enables IRQ lines.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>

void omap_intc_init(void)
{
	/* Mask all 128 IRQ lines (4 banks × 32 bits). */
	mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);

	/* Priority threshold 0xFF = no threshold, all priorities pass through. */
	mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);

	/* Acknowledge any stale IRQ. */
	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);

	printk("[INTC] AM335x INTC ready\n");
}

void intc_enable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_ILR(irq), 0);
	mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

void intc_disable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}
