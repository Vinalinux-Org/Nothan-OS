/* ============================================================
 * vinix/init.h
 * ------------------------------------------------------------
 * Boot-time initcall mechanism — Linux-style level dispatch.
 * ============================================================ */

#ifndef VINIX_INIT_H
#define VINIX_INIT_H

/* Tag function/data as init-only. Lives in .init.* sections —
 * candidate for future free_initmem() reclamation after boot. */
#define __init      __attribute__((section(".init.text"), used))
#define __initdata  __attribute__((section(".init.data"), used))

typedef int (*initcall_t)(void);

/* Place a function pointer into .initcall<level>.init section.
 * do_initcalls(level) iterates that section at boot. */
#define __define_initcall(fn, level) \
    static initcall_t __initcall_##fn##level \
        __attribute__((used, section(".initcall" #level ".init"))) = (fn)

#define core_initcall(fn)      __define_initcall(fn, 1)
#define postcore_initcall(fn)  __define_initcall(fn, 2)
#define arch_initcall(fn)      __define_initcall(fn, 3)
#define subsys_initcall(fn)    __define_initcall(fn, 4)
#define fs_initcall(fn)        __define_initcall(fn, 5)
#define device_initcall(fn)    __define_initcall(fn, 6)
#define late_initcall(fn)      __define_initcall(fn, 7)

/* Convenience for platform drivers — equivalent to a device_initcall
 * that calls platform_driver_register(&__pdrv).
 *
 * Differs from Linux: no module_exit / MODULE_LICENSE pair (VinixOS
 * does not support module unload). Macro name kept for habit pattern. */
#define module_platform_driver(__pdrv) \
    static int __init __pdrv##_init(void) { \
        return platform_driver_register(&(__pdrv)); \
    } \
    device_initcall(__pdrv##_init)

void do_initcalls(int level);

#endif /* VINIX_INIT_H */
