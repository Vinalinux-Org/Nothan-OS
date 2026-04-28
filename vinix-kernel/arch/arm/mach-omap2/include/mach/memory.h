/* ============================================================
 * platform/memory.h
 * ------------------------------------------------------------
 * AM3358 + BeagleBone Black physical memory constants.
 * ============================================================ */

#ifndef PLATFORM_MEMORY_H
#define PLATFORM_MEMORY_H

#define PLATFORM_DDR_PA_BASE    0x80000000
#define PLATFORM_DDR_SIZE_MB    128

/* FB_PA_BASE lies after Kernel (5 MB) + User (1 MB) + 2 MB margin
 * to avoid overlap with kernel/user regions. */
#define PLATFORM_FB_PA_BASE     0x80800000
#define PLATFORM_FB_SIZE_MB     4

#endif
