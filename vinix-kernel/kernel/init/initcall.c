/* ============================================================
 * initcall.c
 * ------------------------------------------------------------
 * Iterate .initcall<N>.init sections, invoke registered fns.
 * ============================================================ */

#include "vinix/init.h"
#include "uart.h"

/* Defined in arch/arm/kernel.ld — bracket each level's section. */
extern initcall_t __initcall1_start[], __initcall1_end[];
extern initcall_t __initcall2_start[], __initcall2_end[];
extern initcall_t __initcall3_start[], __initcall3_end[];
extern initcall_t __initcall4_start[], __initcall4_end[];
extern initcall_t __initcall5_start[], __initcall5_end[];
extern initcall_t __initcall6_start[], __initcall6_end[];
extern initcall_t __initcall7_start[], __initcall7_end[];

static initcall_t *level_start[8] = {
    0,
    __initcall1_start, __initcall2_start, __initcall3_start,
    __initcall4_start, __initcall5_start, __initcall6_start,
    __initcall7_start,
};
static initcall_t *level_end[8] = {
    0,
    __initcall1_end, __initcall2_end, __initcall3_end,
    __initcall4_end, __initcall5_end, __initcall6_end,
    __initcall7_end,
};

void do_initcalls(int level)
{
    if (level < 1 || level > 7) return;

    initcall_t *fn  = level_start[level];
    initcall_t *end = level_end[level];

    while (fn < end) {
        if (*fn) (void)(*fn)();
        fn++;
    }
}
