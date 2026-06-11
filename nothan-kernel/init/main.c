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

	if (vfs_mount("sd0", "fat32") != 0)
		printk("[VFS] SD card mount failed\n");

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
