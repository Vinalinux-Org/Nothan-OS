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

	/* Look for a test file — try HELLO.BIN first, then SHELL.BIN (mock) */
	const char *test_files[] = { "HELLO.BIN", "SHELL.BIN" };
	int found = 0;
	for (unsigned int i = 0; i < sizeof(test_files) / sizeof(test_files[0]); i++) {
		int fd = vfs_open(test_files[i], O_RDONLY);
		if (fd >= 0) {
			char test_buf[64];
			int bytes = vfs_read(fd, test_buf, sizeof(test_buf) - 1);
			if (bytes > 0) {
				test_buf[bytes] = '\0';
				printk("[VFS] Read %d bytes from %s: '%s'\n",
				       bytes, test_files[i], test_buf);
			}
			vfs_close(fd);
			found = 1;
			break;
		}
	}
	if (!found)
		printk("[VFS] No test file found\n");

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
