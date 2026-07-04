#ifndef _MMIO_H
#define _MMIO_H

#include <nothan/types.h>

#define mmio_read32(a)		({ u32 __v = *(volatile u32 *)(a); __v; })
#define mmio_write32(a, v)	do { *(volatile u32 *)(a) = (v); } while (0)
#define mmio_read16(a)		({ u16 __v = *(volatile u16 *)(a); __v; })
#define mmio_write16(a, v)	do { *(volatile u16 *)(a) = (u16)(v); } while (0)
#define mmio_read8(a)		({ u8 __v = *(volatile u8 *)(a); __v; })
#define mmio_write8(a, v)	do { *(volatile u8 *)(a) = (u8)(v); } while (0)

/*
 * MMIO physical-to-virtual address conversion.
 *
 * The MMU maps these peripheral windows (see mmu_init()):
 *   L4_PER:  PA 0x48000000-0x49FFFFFF → VA 0xF0000000-0xF1FFFFFF
 *   L4_WKUP: PA 0x44E00000-0x44FFFFFF → VA 0xF0E00000-0xF0EFFFFF
 *   L4_FAST: PA 0x4A000000-0x4A1FFFFF → VA 0xF2000000-0xF21FFFFF
 *   USB:     PA 0x47400000-0x474FFFFF → VA 0xF3000000-0xF30FFFFF
 *
 * Platform device resources carry physical base addresses.
 * Drivers MUST convert to VA before register access.
 */
#define L4_PER_VA_BASE	0xF0000000UL
#define L4_WKUP_VA_BASE	0xF0E00000UL
#define L4_FAST_VA_BASE	0xF2000000UL
#define USB_VA_BASE	0xF3000000UL

#define L4_PER_PA_BASE	0x48000000UL
#define L4_WKUP_PA_BASE	0x44E00000UL
#define L4_FAST_PA_BASE	0x4A000000UL
#define USB_PA_BASE	0x47400000UL

static inline u32 phys_to_mmio(u32 pa)
{
	if (pa >= L4_PER_PA_BASE && pa < L4_PER_PA_BASE + 0x02000000)
		return pa - L4_PER_PA_BASE + L4_PER_VA_BASE;
	if (pa >= L4_WKUP_PA_BASE && pa < L4_WKUP_PA_BASE + 0x00100000)
		return pa - L4_WKUP_PA_BASE + L4_WKUP_VA_BASE;
	if (pa >= L4_FAST_PA_BASE && pa < L4_FAST_PA_BASE + 0x00200000)
		return pa - L4_FAST_PA_BASE + L4_FAST_VA_BASE;
	if (pa >= USB_PA_BASE && pa < USB_PA_BASE + 0x00100000)
		return pa - USB_PA_BASE + USB_VA_BASE;
	return pa; /* fallback — identity mapping for debug */
}

#endif /* _MMIO_H */
