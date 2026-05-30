#include <nothan/types.h>
#include <nothan/irq.h>

void kernel_main(void)
{
	intc_init();

	while (1)
		;
}
