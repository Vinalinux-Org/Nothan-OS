#ifndef _MMIO_H
#define _MMIO_H

#include <nothan/types.h>

#define mmio_read32(a)		({ u32 __v = *(volatile u32 *)(a); __v; })
#define mmio_write32(a, v)	do { *(volatile u32 *)(a) = (v); } while (0)

/*
 * MMIO physical-to-virtual address conversion.
 *
 * The MMU maps three peripheral windows (see mmu_init()):
 *   L4_PER:  PA 0x48000000-0x49FFFFFF → VA 0xF0000000-0xF1FFFFFF
 *   L4_WKUP: PA 0x44E00000-0x44FFFFFF → VA 0xF0E00000-0xF0EFFFFF
 *   L4_FAST: PA 0x4A000000-0x4A1FFFFF → VA 0xF2000000-0xF21FFFFF
 *
 * Platform device resources carry physical base addresses.
 * Drivers MUST convert to VA before register access.
 */
#define L4_PER_VA_BASE	0xF0000000UL
#define L4_WKUP_VA_BASE	0xF0E00000UL
#define L4_FAST_VA_BASE	0xF2000000UL

#define L4_PER_PA_BASE	0x48000000UL
#define L4_WKUP_PA_BASE	0x44E00000UL
#define L4_FAST_PA_BASE	0x4A000000UL

static inline u32 phys_to_mmio(u32 pa)
{
	if (pa >= L4_PER_PA_BASE && pa < L4_PER_PA_BASE + 0x02000000)
		return pa - L4_PER_PA_BASE + L4_PER_VA_BASE;
	if (pa >= L4_WKUP_PA_BASE && pa < L4_WKUP_PA_BASE + 0x00100000)
		return pa - L4_WKUP_PA_BASE + L4_WKUP_VA_BASE;
	if (pa >= L4_FAST_PA_BASE && pa < L4_FAST_PA_BASE + 0x00200000)
		return pa - L4_FAST_PA_BASE + L4_FAST_VA_BASE;
	return pa; /* fallback — identity mapping for debug */
}

#endif /* _MMIO_H */
