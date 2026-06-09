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

static struct completion test_completion;

/* Test: msleep + udelay — sleep 2s, print timestamp */
static void task_sleep(void)
{
	unsigned long start = get_jiffies();

	printk("sleep start\n");

	while (1) {
		udelay(500);
		msleep(1000);
		unsigned long now = get_jiffies();
		/* print elapsed jiffies (each = 10ms) as decimal */
		printk("t=");
		/* crude: print the elapsed jiffies in decimal by repeated
		   subtraction of 100s */
		unsigned long v = now - start;
		char buf[12];
		int p = 0;
		/* max ~4 digits for reasonable uptime */
		int started = 0;
		for (unsigned long d = 10000; d >= 1; d /= 10) {
			int digit = 0;
			while (v >= d) { v -= d; digit++; }
			if (digit || started || d == 1) {
				buf[p++] = '0' + digit;
				started = 1;
			}
		}
		buf[p] = '\0';
		printk(buf);
		printk("ms\n");
	}
}

/* Test: completion — wait, then print when completed */
static void task_waiter(void)
{
	printk("waiter start\n");
	wait_for_completion(&test_completion);
	printk("waiter done\n");
	while (1)
		schedule();
}

/* Test: kernel timer — fires once, then complete the waiter */
static void timer_callback(struct timer_list *t)
{
	printk("timer fired\n");
	complete(&test_completion);
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

	init_completion(&test_completion);

	struct task_struct *ts = task_create(task_sleep, DEFAULT_PRIO, "sleep");
	struct task_struct *tw = task_create(task_waiter, DEFAULT_PRIO, "waiter");

	if (ts) enqueue_task(&runqueue, ts);
	if (tw) enqueue_task(&runqueue, tw);

	printk("NothanOS v2 kernel started\n");

	/* Arm a one-shot kernel timer: prints + completes after 3 s */
	struct timer_list test_timer;
	init_timer(&test_timer);
	test_timer.expires = get_jiffies() + 3 * HZ;
	test_timer.function = timer_callback;
	add_timer(&test_timer);

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	/*
	 * Hand control to the scheduler.  The idle task (built into sched_init)
	 * is always available, so pick_next_task() never returns NULL.
	 */
	schedule();

	/* NOTREACHED */
	while (1)
		;
}
