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
extern struct task_struct *user_task_create(const char *name);
extern struct task_struct *user_task_create_gui(void);
extern struct task_struct *kernel_spawn(const char *path);

void kernel_main(void)
{
	/* Platform init: board registers devices → drivers match → probe.
	 * UART must be initialized before any printk() calls. */
	do_initcalls();

	page_alloc_init();

	slab_init();

	if (vfs_mount("sda", "fat32") != 0)
		printk("[VFS] SD card mount failed\n");

	vfs_mount(NULL, "devfs");

	mmu_log_config();

	sched_init();

	timer_start();

	struct task_struct *ut = kernel_spawn("/sbin/init");
	if (ut) {
		printk("[KERN] Spawning /sbin/init\n");
		enqueue_task(&runqueue, ut);
	} else {
		printk("[KERN] /sbin/init not found, falling back to embedded shell\n");
		ut = user_task_create("shell");
		if (ut)
			enqueue_task(&runqueue, ut);
	}

	struct task_struct *gui = user_task_create_gui();
	if (gui) {
		printk("[KERN] Spawning embedded GUI\n");
		enqueue_task(&runqueue, gui);
	}

	printk("[KERN] NothanOS started\n");

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
