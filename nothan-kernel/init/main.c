#include <nothan/types.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/sched.h>

#include <nothan/mmio.h>


extern void timer_init(void);

static void task_a(void)
{
	while (1) {
		printk("A");
		for (volatile int i = 0; i < 700000; i++)
			;
		schedule();
		printk(">"); /* checkpoint: schedule() returned to task_a */
	}
}

static void task_b(void)
{
	while (1) {
		printk("B");
		for (volatile int i = 0; i < 700000; i++)
			;
		schedule();
		printk("<"); /* checkpoint: schedule() returned to task_b */
	}
}

void kernel_main(void)
{
	const char *msg = "kmain\n";
	for (unsigned int i = 0; msg[i]; i++) {
		while (!(mmio_read32(UART_BASE + UART_LSR) & (1 << 5)))
			;
		mmio_write32(UART_BASE + UART_THR, msg[i]);
	}

	intc_init();
	printk("a\n");
	 
	uart_init();
	printk("b\n");

	timer_init();
	printk("c\n");

	page_alloc_init();
	printk("d\n");

	slab_init();
	printk("e\n");
	
	sched_init();
	printk("f\n"); 

	struct task_struct *t1 = task_create(task_a, DEFAULT_PRIO, "task_a");
	struct task_struct *t2 = task_create(task_b, DEFAULT_PRIO, "task_b");

	if (t1)
		enqueue_task(&runqueue, t1);
	if (t2)
		enqueue_task(&runqueue, t2);

	printk("NothanOS v2 kernel started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	while (1)
		schedule();
}
