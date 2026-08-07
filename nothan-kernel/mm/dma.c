/*
 * mm/dma.c - Coherent pool allocator and streaming DMA mapping
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * See <nothan/dma.h> for the two shapes of DMA buffer and why the direction
 * argument is not advisory.
 */

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/dma.h>
#include <nothan/printk.h>
#include <asm/memory.h>
#include <asm/barrier.h>
#include <asm/irqflags.h>

#define DMA_POOL_PAGES		(DMA_POOL_SIZE / PAGE_SIZE)	/* 256 */

/*
 * A bitmap, not a buddy allocator.
 *
 * The pool is one megabyte and its customers are a handful of long-lived
 * descriptor rings claimed at probe time and kept for the life of the system.
 * A first-fit scan over 256 bits is a few instructions and cannot fragment in
 * any way that matters at that scale; a second buddy allocator would be more
 * code to reason about for a problem this does not have.
 */
static u32 dma_map_bits[(DMA_POOL_PAGES + 31) / 32];
static unsigned int dma_pages_used;

static inline int bit_test(unsigned int i)
{
	return (dma_map_bits[i / 32] >> (i % 32)) & 1u;
}

static inline void bit_set(unsigned int i)
{
	dma_map_bits[i / 32] |= (1u << (i % 32));
}

static inline void bit_clear(unsigned int i)
{
	dma_map_bits[i / 32] &= ~(1u << (i % 32));
}

void dma_init(void)
{
	/*
	 * No cache maintenance here, and that is a statement about the layout
	 * rather than an omission. The D-cache is switched on in the same
	 * instruction that enables the MMU (__turn_mmu_on), by which time this
	 * section is already mapped non-cacheable - so these addresses have
	 * never been cacheable and there is nothing that could be holding a copy
	 * of them.
	 *
	 * The pool is not zeroed either: it is a megabyte of UNCACHED writes,
	 * which is slow enough to be visible in boot time, and most of it would
	 * be zeroing memory nobody asks for. Each allocation clears its own
	 * pages instead - see dma_alloc_coherent(), where it is not optional,
	 * because a descriptor ring built on stale DRAM hands a device
	 * addresses left over from the last boot.
	 */
	printk("[DMA] coherent pool %luKB at PA 0x%lx (VA 0x%lx, uncached)\n",
	       DMA_POOL_SIZE >> 10, DMA_POOL_PA, DMA_POOL_VA);
}

void *dma_alloc_coherent(unsigned long size, unsigned long *dma_handle)
{
	unsigned long flags;
	unsigned int need, run = 0, start = 0;
	int found = -1;

	if (!size)
		return NULL;

	need = (unsigned int)((size + PAGE_SIZE - 1) / PAGE_SIZE);

	local_irq_save(flags);
	for (unsigned int i = 0; i < DMA_POOL_PAGES; i++) {
		if (bit_test(i)) {
			run = 0;
			continue;
		}
		if (run == 0)
			start = i;
		if (++run == need) {
			found = (int)start;
			break;
		}
	}
	if (found >= 0) {
		for (unsigned int i = 0; i < need; i++)
			bit_set((unsigned int)found + i);
		dma_pages_used += need;
	}
	local_irq_restore(flags);

	if (found < 0) {
		/*
		 * The pool does not grow. That is deliberate - a fixed ceiling
		 * is the point - so this is a design overflow rather than a
		 * transient shortage, and the message says how much was asked
		 * for and how much exists so the ceiling can be raised on
		 * purpose instead of by trial.
		 */
		pr_err("[DMA] coherent alloc of %lu B failed: %u/%u pages used\n",
		       size, dma_pages_used, (unsigned int)DMA_POOL_PAGES);
		return NULL;
	}

	unsigned long off = (unsigned long)found * PAGE_SIZE;
	unsigned long va  = DMA_POOL_VA + off;

	/*
	 * Zero it. Not tidiness: a descriptor ring is read by hardware the
	 * moment the device is told where it is, and whatever DRAM happens to
	 * hold at that address will be interpreted as buffer pointers and
	 * lengths. Uninitialised memory here means a DMA engine writing to
	 * addresses left over from the previous boot.
	 */
	for (unsigned long i = 0; i < (unsigned long)need * PAGE_SIZE / 4; i++)
		((volatile u32 *)va)[i] = 0;

	if (dma_handle)
		*dma_handle = DMA_POOL_PA + off;

	return (void *)va;
}

void dma_free_coherent(void *va, unsigned long size)
{
	unsigned long flags;
	unsigned long addr = (unsigned long)va;
	unsigned int first, need;

	if (!va || !size)
		return;

	if (addr < DMA_POOL_VA || addr >= DMA_POOL_VA + DMA_POOL_SIZE ||
	    (addr & (PAGE_SIZE - 1))) {
		pr_err("[DMA] dma_free_coherent(%p): not a coherent allocation\n",
		       va);
		return;
	}

	first = (unsigned int)((addr - DMA_POOL_VA) / PAGE_SIZE);
	need  = (unsigned int)((size + PAGE_SIZE - 1) / PAGE_SIZE);

	if (first + need > DMA_POOL_PAGES) {
		pr_err("[DMA] dma_free_coherent(%p, %lu): runs past the pool\n",
		       va, size);
		return;
	}

	local_irq_save(flags);
	for (unsigned int i = 0; i < need; i++)
		bit_clear(first + i);
	dma_pages_used -= need;
	local_irq_restore(flags);
}

unsigned long dma_pool_free_bytes(void)
{
	return (unsigned long)(DMA_POOL_PAGES - dma_pages_used) * PAGE_SIZE;
}

/*
 * Streaming maps. The cache work is all there is to these - there is no IOMMU,
 * so the "mapping" is just the direct-map arithmetic.
 */
static void dma_sync(unsigned long start, unsigned long size, int dir)
{
	unsigned long end = start + size;

	switch (dir) {
	case DMA_TO_DEVICE:
		/* The device is about to read DRAM. Anything the CPU wrote may
		 * still be sitting dirty in the cache; push it out. */
		clean_dcache_range(start, end);
		break;
	case DMA_FROM_DEVICE:
		/* The device is about to overwrite DRAM. Drop what the cache
		 * holds now, or a later read returns the pre-transfer contents
		 * - and a speculative fetch during the transfer would do the
		 * same. Invalidating up front is why this runs on map as well
		 * as on unmap. */
		invalidate_dcache_range(start, end);
		break;
	default:
		flush_dcache_range(start, end);
		break;
	}
}

unsigned long dma_map_single(void *va, unsigned long size, int dir)
{
	unsigned long addr = (unsigned long)va;
	unsigned long pa;

	if (!va || !size)
		return 0;

	/*
	 * Only direct-mapped kernel memory can be given to a device: the
	 * controller addresses DRAM physically and has no idea what a page
	 * table is. A kernel stack (0xE0000000 window), a user buffer, or an
	 * MMIO alias would all produce a plausible-looking number here and send
	 * a device writing somewhere unrelated - so the range is checked rather
	 * than assumed.
	 */
	if (addr < PAGE_OFFSET || addr >= DMA_POOL_VA ||
	    addr + size > DMA_POOL_VA || addr + size < addr) {
		pr_err("[DMA] dma_map_single(%p, %lu): not direct-mapped kernel memory\n",
		       va, size);
		return 0;
	}

	dma_sync(addr, size, dir);

	pa = __virt_to_phys(addr);
	return pa;
}

void dma_unmap_single(void *va, unsigned long size, int dir)
{
	unsigned long addr = (unsigned long)va;

	if (!va || !size)
		return;

	/*
	 * Nothing to do on the way out for a buffer the device only READ - the
	 * cache cannot have gone stale over data nobody wrote. For the other
	 * two directions the device has just written DRAM behind the cache's
	 * back, and this is the invalidate that makes the new data visible.
	 */
	if (dir == DMA_TO_DEVICE)
		return;

	invalidate_dcache_range(addr, addr + size);
}
