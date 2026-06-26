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

extern void mmu_log_config(void);
extern void omap_intc_init(void);
extern struct task_struct *user_task_create(const char *name);
extern struct task_struct *user_task_create_gui(void);

/*
 * Set to 1 to run the FAT32 write self-test at boot. Pure UART output —
 * needs no touchscreen or shell. Verified PASS on BBB 2026-06-21
 * (round-trip + boot counter persists across reboots); left off now.
 */
#define FAT_WRITE_SELFTEST  0

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
	 * Mask IRQs while spawning all initial tasks. Timer IRQ is already
	 * running (10ms tick), and would otherwise preempt kernel_main into
	 * the first enqueued task before later tasks (GUI) get created.
	 */
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	/* BOOT_GUI: 1 spawns the LVGL GUI (+ shell). 0 skips it (blank screen),
	 * useful when bringing up lower layers without the GUI on top. */
#define BOOT_GUI 1
#if BOOT_GUI
	struct task_struct *gui = user_task_create_gui();
	if (gui) {
		printk("[KERN] Spawning GUI\n");
		enqueue_task(&runqueue, gui);
	}

	struct task_struct *sh = user_task_create("shell");
	if (sh) {
		printk("[KERN] Spawning shell\n");
		enqueue_task(&runqueue, sh);
	}
#else
	printk("[KERN] BOOT_GUI=0: GUI not spawned\n");
#endif

	printk("[KERN] NothanOS started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
