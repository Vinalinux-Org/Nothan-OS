#include <nothan/types.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/sched.h>
#include <nothan/delay.h>
#include <nothan/timer.h>
#include <nothan/completion.h>
#include <nothan/time.h>

#include <nothan/mmio.h>


extern void timer_init(void);
extern void mmu_log_config(void);

static struct completion test_completion;

/* Test: msleep — prints jiffies timer */
static void task_sleep(void)
{
	unsigned long start = get_jiffies();

	printk("[TEST] msleep start (t=0)\n");

	while (1) {
		udelay(500);
		msleep(1000);
		printk("[TEST] t=%lums\n", (get_jiffies() - start) * 10);
	}
}

/* Test: completion — wait, then print when completed */
static void task_waiter(void)
{
	printk("[TEST] waiter waiting...\n");
	wait_for_completion(&test_completion);
	printk("[TEST] waiter done!\n");
	while (1)
		schedule();
}

/* Test: kernel timer fires once, then completes the waiter */
static void timer_callback(struct timer_list *t)
{
	printk("[TEST] one-shot timer fired\n");
	complete(&test_completion);
}


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

	init_completion(&test_completion);

	struct task_struct *ts = task_create(task_sleep, DEFAULT_PRIO, "sleep");
	struct task_struct *tw = task_create(task_waiter, DEFAULT_PRIO, "waiter");

	if (ts) enqueue_task(&runqueue, ts);
	if (tw) enqueue_task(&runqueue, tw);

	printk("[KERN] NothanOS started\n");

	/* Arm a one-shot kernel timer: fires after 3 s. */
	struct timer_list test_timer;
	init_timer(&test_timer);
	test_timer.expires = get_jiffies() + 3 * HZ;
	test_timer.function = timer_callback;
	add_timer(&test_timer);

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
