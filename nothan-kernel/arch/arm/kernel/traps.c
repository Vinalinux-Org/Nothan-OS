#include <nothan/types.h>
#include <nothan/irq.h>

void und_handler(unsigned int spsr)
{
	while (1)
		;
}

void svc_handler(unsigned int spsr)
{
	while (1)
		;
}

void pabt_handler(unsigned int spsr)
{
	while (1)
		;
}

void dabt_handler(unsigned int spsr)
{
	while (1)
		;
}

void irq_handler(unsigned int spsr)
{
	intc_handle_irq();
}

void fiq_handler(unsigned int spsr)
{
	while (1)
		;
}
