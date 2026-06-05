#include <nothan/types.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/sched.h>

extern void timer_init(void);

static void task_a(void)
{
	while (1) {
		printk("A");
		for (volatile int i = 0; i < 500000; i++)
			;
		schedule();
	}
}

static void task_b(void)
{
	while (1) {
		printk("B");
		for (volatile int i = 0; i < 500000; i++)
			;
		schedule();
	}
}

void kernel_main(void)
{
	intc_init();
	uart_init();
	timer_init();

	page_alloc_init();
	slab_init();
	sched_init();

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
