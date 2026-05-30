#include <nothan/types.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/printk.h>

extern void timer_init(void);

void kernel_main(void)
{
	intc_init();
	uart_init();
	timer_init();

	printk("NothanOS v2 kernel started\n");

	/* Enable IRQ at CPU level. */
	__asm__ __volatile__ ("cpsie i" : : : "memory");

	while (1)
		;
}
