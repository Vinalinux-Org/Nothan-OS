#ifndef _NOTHAN_INIT_H
#define _NOTHAN_INIT_H

/*
 * Initcall mechanism
 *
 * Drivers register init functions into the .initcall section via macros.
 * do_initcalls() iterates the section in priority order at boot.
 *
 * Priority levels (lower number = earlier):
 *   0  arch          — arch-level init (MMU, IRQ controller)
 *   1  subsys        — subsystem infrastructure (buses, timers)
 *   2  fs            — filesystem layer
 *   3  device        — drivers (UART, MMC, GPIO...)
 *   4  late          — everything else
 */

typedef int (*initcall_t)(void);

#define __init __attribute__((__section__(".init.text")))

#define __initcall_fn(fn, lvl) \
	static initcall_t __initcall_##fn __attribute__((__section__(".initcall." #lvl), __used__)) = fn

#define early_initcall(fn)   __initcall_fn(fn, early)
#define arch_initcall(fn)    __initcall_fn(fn, 0)
#define subsys_initcall(fn)  __initcall_fn(fn, 1)
#define fs_initcall(fn)      __initcall_fn(fn, 2)
#define device_initcall(fn)  __initcall_fn(fn, 3)
#define late_initcall(fn)    __initcall_fn(fn, 4)

/* Called from kernel_main: scans .initcall section and runs all entries. */
void do_initcalls(void);

#endif /* _NOTHAN_INIT_H */
