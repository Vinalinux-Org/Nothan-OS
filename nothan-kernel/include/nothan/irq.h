#ifndef _IRQ_H
#define _IRQ_H

/*
 * INTC is at PA 0x48200000, mapped to VA 0xF0200000
 * via the MMU's L4_PER section (VA 0xF0000000 → PA 0x48000000).
 */
#define INTC_BASE		0xF0200000

#define INTC_SIR_IRQ		0x40
#define INTC_SIR_FIQ		0x44
#define INTC_CONTROL		0x48
#define INTC_THRESHOLD		0x68
#define INTC_MIR(n)		(0x84 + ((n) * 0x20))
#define INTC_MIR_CLEAR(n)	(0x88 + ((n) * 0x20))
#define INTC_MIR_SET(n)		(0x8C + ((n) * 0x20))
#define INTC_ILR(n)		(0x100 + ((n) * 4))

#define NEWIRQAGR		(1 << 0)

#define NR_IRQS			128

/* Interrupt handler type. */
typedef void (*irq_handler_t)(unsigned int irq);

void intc_init(void);
void intc_enable_irq(unsigned int irq);
void intc_disable_irq(unsigned int irq);
void intc_handle_irq(void);
void request_irq(unsigned int irq, irq_handler_t handler);

#endif /* _IRQ_H */
