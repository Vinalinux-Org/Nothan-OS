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
#include <nothan/block.h>
#include <nothan/fs.h>

extern void mmu_log_config(void);
extern struct task_struct *user_task_create(const char *name);

void kernel_main(void)
{
	/* Platform init: board registers devices → drivers match → probe.
	 * UART must be initialized before any printk() calls. */
	do_initcalls();

	page_alloc_init();

	slab_init();

	mock_bdev_init();

	/* Try real SD card (sd0). Fall back to mock device if no card inserted. */
	if (vfs_mount("sd0", "fat32") != 0) {
		printk("[VFS] No SD card, using mock device\n");
		if (vfs_mount("mock0", "fat32") != 0) {
			printk("[VFS] No block device available\n");
		}
	}

	/* Smoke-test VFS: open SHELL.BIN and read first bytes. */
	{
		int fd = vfs_open("SHELL.BIN", O_RDONLY);
		if (fd >= 0) {
			char buf[64];
			int n = vfs_read(fd, buf, 63);
			if (n > 0) {
				buf[n] = '\0';
				printk("[VFS] Read %d bytes from SHELL.BIN: '%s'\n", n, buf);
			}
			vfs_close(fd);
		}
	}

	mmu_log_config();

	sched_init();

	timer_start();

	struct task_struct *ut = user_task_create("shell");
	if (ut)
		enqueue_task(&runqueue, ut);

	printk("[KERN] NothanOS started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
