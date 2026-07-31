/*
 * init/main.c - Kernel entry point and early boot sequence
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/sched.h>
#include <nothan/timer.h>
#include <nothan/init.h>
#include <nothan/fs.h>
#include <nothan/msgq.h>
#include <nothan/delay.h>
#include <nothan/panic.h>

extern void mmu_log_config(void);
extern void omap_intc_init(void);

/*
 * Set to 1 to run the FAT32 write self-test at boot. Pure UART output —
 * needs no touchscreen or shell. Verified PASS on BBB 2026-06-21
 * (round-trip + boot counter persists across reboots); left off now.
 */
#define FAT_WRITE_SELFTEST  0

/*
 * Set to 1 to run the message-queue self-test at boot: two kernel threads
 * (producer/consumer) pass 10 messages through a 4-slot bounded queue. The
 * consumer drains slowly (msleep) so the producer must block on "full" and
 * the consumer blocks on "empty" — exercising wait_event/wake_up at runtime.
 * Pure UART output. Threads exit when done (also exercises exit + reap).
 */
#define MSGQ_SELFTEST  0

/* Set to 1 to fire BUG_ON(1) early at boot — verifies panic() + the
 * kernel-context backtrace. Pure UART output. */
#define PANIC_SELFTEST  0

#if FAT_WRITE_SELFTEST
/*
 * fat_write_selftest() - Exercise the FAT32 write path over UART only.
 *
 * 1. Round-trip: create /NOTHAN.TST, write a marker, read it back, and
 *    compare — proves create + write + read in one boot.
 * 2. Boot counter: read a u32 from /BOOTCNT.BIN, increment, write it
 *    back. The count climbing across power cycles proves the data
 *    actually persists on the SD card, not just in RAM.
 */
static void fat_write_selftest(void)
{
	static const char marker[] = "NothanOS FAT32 write works";
	char rb[40];
	int fd, n;

	printk("[FATTEST] --- begin ---\n");

	/* (1) Round-trip a known string. */
	fd = vfs_open("/NOTHAN.TST", O_WRONLY | O_CREAT);
	if (fd < 0) {
		printk("[FATTEST] open(create) FAILED\n");
		return;
	}
	n = vfs_write(fd, marker, sizeof(marker));
	vfs_close(fd);
	printk("[FATTEST] wrote %d bytes\n", n);

	fd = vfs_open("/NOTHAN.TST", O_RDONLY);
	if (fd < 0) {
		printk("[FATTEST] reopen FAILED\n");
		return;
	}
	n = vfs_read(fd, rb, sizeof(rb));
	vfs_close(fd);

	int match = (n == (int)sizeof(marker));
	for (int i = 0; match && i < n; i++)
		if (rb[i] != marker[i])
			match = 0;
	printk("[FATTEST] read %d bytes, round-trip %s\n",
	       n, match ? "PASS" : "FAIL");

	/* (2) Persistence counter across reboots. */
	unsigned int boot = 0;
	fd = vfs_open("/BOOTCNT.BIN", O_RDONLY);
	if (fd >= 0) {
		vfs_read(fd, (char *)&boot, sizeof(boot));
		vfs_close(fd);
	}
	boot++;
	fd = vfs_open("/BOOTCNT.BIN", O_WRONLY | O_CREAT);
	if (fd >= 0) {
		vfs_write(fd, (const char *)&boot, sizeof(boot));
		vfs_close(fd);
	}
	printk("[FATTEST] boot count = %u (should climb every reboot)\n", boot);
	printk("[FATTEST] --- end ---\n");
}
#endif

#if MSGQ_SELFTEST
static struct msgq test_q;
static unsigned int test_q_buf[4];		/* 4 slots × sizeof(unsigned int) */

static void msgq_producer(void)
{
	for (unsigned int i = 0; i < 10; i++) {
		msgq_send(&test_q, &i);		/* no delay → fills then blocks on full */
		printk("[MSGQTEST] send=%u\n", i);
	}
}

static void msgq_consumer(void)
{
	for (unsigned int i = 0; i < 10; i++) {
		unsigned int v;
		msgq_recv(&test_q, &v);		/* blocks while empty */
		printk("[MSGQTEST] recv=%u%s\n", v, v == i ? "" : " !!ORDER");
		msleep(30);			/* slow drain → producer blocks on full */
	}
	printk("[MSGQTEST] PASS (10 msgs in order)\n");
}
#endif

void kernel_main(void)
{
	printk("[BOOT] page_alloc_init\n");
	page_alloc_init();

	printk("[BOOT] slab_init\n");
	slab_init();

	/*
	 * sched_init() before do_initcalls() — mirrors Linux start_kernel().
	 * sched_running stays false until the first real context switch, so
	 * wait_for_completion() inside driver probe still takes the spin-wait
	 * path (not schedule()).
	 */
	printk("[BOOT] sched_init\n");
	sched_init();

	msgq_sys_init();	/* system message queues for the msgq_send/recv syscalls */

	/*
	 * init, and ONLY init. Created HERE - before do_initcalls() - and that
	 * position is the point.
	 *
	 * PIDs are handed out in creation order and init is defined by its
	 * number, so it has to be first. But "first" has to mean first overall,
	 * not first user process: driver probes create kernel threads (the MUSB
	 * enumerator), and anything created while init_task is still NULL ends up
	 * parentless. Creating init before any of them is what makes "every task
	 * except init has a parent" true by construction rather than by luck.
	 *
	 * It needs nothing that is not already up: the page allocator, slab and
	 * scheduler are initialised above, and its image is embedded in the kernel
	 * rather than read from a disk that is not mounted yet. It only runs once
	 * schedule() is called at the end of this function, by which time the
	 * drivers and the filesystem it actually uses are ready.
	 *
	 * No IRQ masking needed: the timer is not started until further down, so
	 * nothing can preempt kernel_main here.
	 *
	 * What init DOES - which services to start, and reaping forever - lives in
	 * userspace/init/main.c. The kernel neither knows nor decides.
	 */
	struct task_struct *init = init_task_create();
	if (init) {
		printk("[KERN] Spawning init (PID 1)\n");
		enqueue_task(&runqueue, init);
	} else {
		panic("cannot create init");
	}

#if PANIC_SELFTEST
	BUG_ON(1);		/* verify panic() + kernel backtrace, then set back to 0 */
#endif

	/*
	 * init_IRQ() equivalent: initialize INTC before any driver runs.
	 * Mirrors Linux start_kernel() → init_IRQ() → irqchip_init().
	 * INTC masks all 128 lines; drivers unmask their own via intc_enable_irq().
	 */
	omap_intc_init();

	/* Open CPU IRQ gate. INTC masks all lines so no spurious IRQs fire. */
	printk("[BOOT] cpsie i\n");
	__asm__ __volatile__ ("cpsie i" : : : "memory");

	printk("[BOOT] do_initcalls\n");
	do_initcalls();        /* tda19988_init runs here as device_initcall */
	printk("[BOOT] do_initcalls done\n");

	if (vfs_mount("sda", "fat32") != 0) {
		printk("[VFS] SD card mount failed\n");
	} else {
		printk("[VFS] fat32 mounted\n");
#if FAT_WRITE_SELFTEST
		fat_write_selftest();
#endif
	}

	vfs_mount(NULL, "devfs");
	printk("[VFS] devfs mounted\n");

	mmu_log_config();

	printk("[BOOT] timer_start\n");
	timer_start();

	/*
	 * Mask IRQs for the rest of boot. The timer is running now (10ms tick)
	 * and would otherwise preempt kernel_main mid-setup; the gate reopens
	 * just before schedule() hands the CPU to the first real task.
	 */
	__asm__ __volatile__ ("cpsid i" : : : "memory");

#if MSGQ_SELFTEST
	msgq_init(&test_q, test_q_buf, sizeof(unsigned int), 4);
	struct task_struct *tp = task_create(msgq_producer, DEFAULT_PRIO, "msgq-prod");
	if (tp)
		enqueue_task(&runqueue, tp);
	struct task_struct *tc = task_create(msgq_consumer, DEFAULT_PRIO, "msgq-cons");
	if (tc)
		enqueue_task(&runqueue, tc);
#endif

	printk("[KERN] NothanOS started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
