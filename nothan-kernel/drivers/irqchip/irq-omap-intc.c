#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>

void intc_init(void)
{
	/* Mask all interrupts. */
	mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);

	/* No threshold (allow all priorities). */
	mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);

	/* New IRQ aggregation required — clear pending. */
	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}

void intc_enable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	/* Set priority to 0 (highest) via ILR. */
	mmio_write32(INTC_BASE + INTC_ILR(irq), 0);

	/* Clear MIR mask to enable. */
	mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

void intc_disable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}
