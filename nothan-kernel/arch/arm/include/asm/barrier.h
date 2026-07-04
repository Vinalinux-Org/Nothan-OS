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
 * clean_dcache_range(start, end) — clean dirty D-cache lines to DRAM so
 * DMA can see CPU writes. Uses clean-by-MVA-to-PoC (c7, c10, 1).
 * Cortex-A8 cache line = 64 bytes.
 */
#define DCACHE_LINE_SIZE	64U
static inline void clean_dcache_range(unsigned long start, unsigned long end)
{
	unsigned long addr = start & ~(DCACHE_LINE_SIZE - 1UL);

	while (addr < end) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1"
			: : "r" (addr) : "memory");
		addr += DCACHE_LINE_SIZE;
	}
	dsb();
}

#endif /* _NOTHAN_BARRIER_H */
