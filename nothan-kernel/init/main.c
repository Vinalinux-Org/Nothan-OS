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
extern struct task_struct *kernel_spawn(const char *path);

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

	if (vfs_mount("sda", "fat32") != 0)
		printk("[VFS] SD card mount failed\n");
	else
		printk("[VFS] fat32 mounted\n");

	vfs_mount(NULL, "devfs");
	printk("[VFS] devfs mounted\n");

	mmu_log_config();

	printk("[BOOT] timer_start\n");
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

	__asm__ __volatile__ ("cpsie i" : : : "memory");  /* re-enable for scheduler */

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
