#include <nothan/types.h>
#include <nothan/irq.h>

/**
 * und_handler - handle undefined instruction exception
 * @spsr: saved program status register at fault
 */
void und_handler(unsigned int spsr)
{
	(void)spsr;
	while (1)
		;
}

/**
 * svc_handler - handle supervisor call (syscall)
 * @spsr: saved program status register at call
 */
void svc_handler(unsigned int spsr)
{
	(void)spsr;
	while (1)
		;
}

/**
 * pabt_handler - handle prefetch abort
 * @spsr: saved program status register at fault
 */
void pabt_handler(unsigned int spsr)
{
	(void)spsr;
	while (1)
		;
}

/**
 * dabt_handler - handle data abort
 * @spsr: saved program status register at fault
 */
void dabt_handler(unsigned int spsr)
{
	(void)spsr;
	while (1)
		;
}

/**
 * irq_handler - top-level interrupt handler
 * @spsr: saved program status register at interrupt
 *
 * Delegates to the INTC dispatch which reads the active IRQ
 * and calls the registered handler.
 */
void irq_handler(unsigned int spsr)
{
	(void)spsr;
	intc_handle_irq();
}

/**
 * fiq_handler - handle fast interrupt request
 * @spsr: saved program status register at interrupt
 */
void fiq_handler(unsigned int spsr)
{
	(void)spsr;
	while (1)
		;
}
