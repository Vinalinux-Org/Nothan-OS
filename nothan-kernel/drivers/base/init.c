/*
 * init.c - Initcall dispatcher — iterates .initcall section
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/init.h>
#include <nothan/printk.h>

/*
 * Linker symbols defining the .initcall section boundaries.
 * Defined in arch/arm/kernel/kernel.ld
 */
extern initcall_t __initcall_start[];
extern initcall_t __initcall_end[];

/**
 * do_initcalls() - Run all registered initcall functions
 *
 * Iterates the .initcall section (populated by arch_initcall,
 * subsys_initcall, device_initcall macros) and calls each function.
 * Non-zero return values are logged but do not abort the sequence.
 */
void do_initcalls(void)
{
	printk("[INIT] Running initcalls...\n");

	for (initcall_t *fn = __initcall_start; fn < __initcall_end; fn++) {
		if (*fn) {
			int ret = (*fn)();
			if (ret)
				printk("[INIT] initcall @%p returned %d\n",
				       *fn, ret);
		}
	}

	printk("[INIT] All initcalls done\n");
}
