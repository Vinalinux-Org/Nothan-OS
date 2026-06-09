/*
 * kernel/irq/irq_core.c - IRQ descriptor table and handler dispatch
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>

static irq_handler_t irq_handlers[NR_IRQS];

/**
 * request_irq() - Register an interrupt handler
 * @irq: The interrupt number to register for
 * @handler: The function to be called when the IRQ occurs
 */
void request_irq(unsigned int irq, irq_handler_t handler)
{
	if (irq < NR_IRQS)
		irq_handlers[irq] = handler;
}

/**
 * intc_handle_irq() - Top-level IRQ handler
 *
 * Called by the assembly IRQ vector. Reads the active IRQ number
 * from the INTC, calls the registered handler, and acknowledges the IRQ.
 */
void intc_handle_irq(void)
{
	u32 sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
	u32 irq = sir & 0x7F;

	if (irq_handlers[irq])
		irq_handlers[irq](irq);

	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}
