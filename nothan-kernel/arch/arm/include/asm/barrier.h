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
 * D-cache maintenance by MVA to the Point of Coherency. Cortex-A8 line = 64 B.
 *
 * There are three operations because DMA has three directions, and using the
 * wrong one is silent:
 *
 *   clean       (c7, c10, 1)  push dirty lines out to DRAM   - CPU wrote, device will read
 *   invalidate  (c7, c6,  1)  drop stale lines from the cache - device wrote, CPU will read
 *   flush       (c7, c14, 1)  clean AND invalidate            - both directions
 *
 * Only clean existed here, which was enough while the only DMA in the kernel
 * was the framebuffer - a buffer the CPU writes and the LCD reads, and never
 * the other way. Every device coming next writes INTO memory: the Ethernet
 * receive ring, the audio capture path, a camera. For those the CPU must throw
 * away what its cache thinks it knows, and with no way to do that it would
 * read whatever happened to be cached from before the transfer - sometimes
 * right, sometimes not, depending on what else touched the buffer. That is the
 * non-reproducible class, and a UART cannot follow it.
 */
#define DCACHE_LINE_SIZE	64U

#define __dc_clean(a)	__asm__ __volatile__("mcr p15, 0, %0, c7, c10, 1" \
					     : : "r" (a) : "memory")
#define __dc_inv(a)	__asm__ __volatile__("mcr p15, 0, %0, c7, c6, 1" \
					     : : "r" (a) : "memory")
#define __dc_flush(a)	__asm__ __volatile__("mcr p15, 0, %0, c7, c14, 1" \
					     : : "r" (a) : "memory")

static inline void clean_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	while (addr < end) {
		__dc_clean(addr);
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

/*
 * invalidate_dcache_range() - discard cached copies so a DMA write is seen
 *
 * The partial lines at each end are CLEANED AND INVALIDATED rather than simply
 * invalidated, and that detail is the whole difficulty of this operation.
 *
 * A cache line is 64 bytes. If a buffer does not start and end on a line
 * boundary, the first and last lines also hold bytes belonging to whatever
 * sits next to the buffer in memory. Plainly invalidating such a line throws
 * away the only copy of those neighbouring bytes if they were dirty - so
 * receiving a packet would corrupt an unrelated variable that happened to
 * share the line. Cleaning them first writes the neighbour's data out before
 * the line is dropped.
 *
 * Callers should still align DMA buffers to a line where they can; this makes
 * the unaligned case safe rather than free.
 */
static inline void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	for (; addr < end; addr += DCACHE_LINE_SIZE) {
		if (addr < start || addr + DCACHE_LINE_SIZE > end)
			__dc_flush(addr);	/* straddles the buffer edge */
		else
			__dc_inv(addr);
	}
	dsb();
}

/* Clean and invalidate — for buffers a device both reads and writes. */
static inline void flush_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	while (addr < end) {
		__dc_flush(addr);
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

#endif /* _NOTHAN_BARRIER_H */
