/*
 * init/main.c - Kernel entry point and early boot sequence
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/sched.h>

extern void timer_init(void);
extern void mmu_log_config(void);
extern struct task_struct *user_task_create(const char *name);

void kernel_main(void)
{
	intc_init();
	printk("[INTC] AM335x INTC ready\n");

	uart_init();
	printk("[UART] 115200 8N1\n");

	timer_init();

	page_alloc_init();

	slab_init();

	mmu_log_config();

	sched_init();

	struct task_struct *ut = user_task_create("user1");
	if (ut)
		enqueue_task(&runqueue, ut);

	printk("[KERN] NothanOS started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
