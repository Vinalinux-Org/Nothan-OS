#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>

/**
 * intc_init() - Initialize the Interrupt Controller
 *
 * Masks all interrupts, allows all priorities, and clears pending aggregations.
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

/**
 * intc_enable_irq() - Unmask an interrupt
 * @irq: The IRQ number to enable
 */
void intc_enable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_ILR(irq), 0);
	mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

/**
 * intc_disable_irq() - Mask an interrupt
 * @irq: The IRQ number to disable
 */
void intc_disable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}
