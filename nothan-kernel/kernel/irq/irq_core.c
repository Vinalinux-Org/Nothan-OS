#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>

static irq_handler_t irq_handlers[NR_IRQS];

void request_irq(unsigned int irq, irq_handler_t handler)
{
	if (irq < NR_IRQS)
		irq_handlers[irq] = handler;
}

void intc_handle_irq(void)
{
	u32 sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
	u32 irq = sir & 0x7F;

	if (irq_handlers[irq])
		irq_handlers[irq](irq);

	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
}
