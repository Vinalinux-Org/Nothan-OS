#ifndef _NOTHAN_BARRIER_H
#define _NOTHAN_BARRIER_H

#define dsb()	__asm__ __volatile__ ("dsb" : : : "memory")
#define dmb()	__asm__ __volatile__ ("dmb" : : : "memory")
#define isb()	__asm__ __volatile__ ("isb" : : : "memory")

/* CP15 register access */
#define read_sctlr()	({ unsigned int __v; \
	__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r" (__v)); __v; })

#define write_sctlr(v)	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0" \
	: : "r" (v) : "memory")

#define read_ttbr0()	({ unsigned int __v; \
	__asm__ __volatile__ ("mrc p15, 0, %0, c2, c0, 0" : "=r" (__v)); __v; })

#define write_ttbr0(v)	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0" \
	: : "r" (v) : "memory")

#define write_ttbcr(v)	__asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 2" \
	: : "r" (v) : "memory")

#define read_dacr()		({ unsigned int __v; \
	__asm__ __volatile__ ("mrc p15, 0, %0, c3, c0, 0" : "=r" (__v)); __v; })

#define write_dacr(v)	__asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0" \
	: : "r" (v) : "memory")

#define flush_tlb()		__asm__ __volatile__ ( \
	"mov r0, #0\n" \
	"mcr p15, 0, r0, c8, c7, 0" : : : "r0", "memory")

#define invalidate_icache()	__asm__ __volatile__ ( \
	"mov r0, #0\n" \
	"mcr p15, 0, r0, c7, c5, 0" : : : "r0", "memory")

/*
 * D-cache maintenance for DMA.
 *
 * DDR is mapped write-back write-allocate, and a DMA engine reads and writes
 * physical memory without going through the cache.  So the two directions need
 * opposite things and neither is optional:
 *
 *   CPU wrote, device will read   — clean, or the device reads stale memory
 *   Device wrote, CPU will read   — invalidate, or the CPU reads a stale line
 *
 * Cortex-A8's L2 is inside the core, so maintenance to the point of coherency
 * reaches it; there is no separate outer-cache controller to drive here, which
 * there would be on a Cortex-A9 with a PL310.
 *
 * Cache line = 64 bytes.
 */
#define DCACHE_LINE_SIZE	64U

#define dccmvac(a)	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1" \
				: : "r" (a) : "memory")	/* clean            */
#define dcimvac(a)	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 1"  \
				: : "r" (a) : "memory")	/* invalidate       */
#define dccimvac(a)	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c14, 1" \
				: : "r" (a) : "memory")	/* clean+invalidate */

/*
 * clean_dcache_range(start, end) — push CPU writes out where a device can see
 * them.  Rounding outwards is harmless in this direction: writing a
 * neighbour's clean data back to the address it already holds changes nothing.
 */
static inline void clean_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	while (addr < end) {
		dccmvac(addr);
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

/*
 * invalidate_dcache_range(start, end) — discard cached copies so the CPU sees
 * what a device wrote.
 *
 * Rounding outwards is NOT harmless here.  A line that straddles the end of
 * the range holds bytes belonging to something else, and invalidating it
 * throws away that owner's unwritten changes — a loss with no fault, in a
 * variable nobody was looking at.  So the two partial lines at the ends are
 * cleaned and invalidated instead, which pushes the neighbour's data to memory
 * before dropping it; only whole lines inside the range are invalidated
 * outright.
 *
 * Callers should still align DMA buffers to a cache line and pad their length,
 * which makes both partial cases disappear.  This handles them anyway, because
 * the alternative is that forgetting shows up as corruption somewhere else
 * entirely.
 */
static inline void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	if (start & (DCACHE_LINE_SIZE - 1UL)) {
		dccimvac(addr);
		addr += DCACHE_LINE_SIZE;
	}

	if (end & (DCACHE_LINE_SIZE - 1UL)) {
		unsigned long tail = end & ~(DCACHE_LINE_SIZE - 1UL);

		if (tail >= addr)
			dccimvac(tail);
		end = tail;
	}

	while (addr < end) {
		dcimvac(addr);
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

/* Both at once, for a buffer that was written and is about to be reused. */
static inline void flush_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	while (addr < end) {
		dccimvac(addr);
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

#endif /* _NOTHAN_BARRIER_H */
