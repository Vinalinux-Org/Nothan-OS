#ifndef _NOTHAN_DMA_H
#define _NOTHAN_DMA_H

#include <nothan/types.h>
#include <nothan/mm.h>

/*
 * DMA buffers, in the two shapes devices actually need.
 *
 * Until now there was no DMA at all: every driver here moves data with the CPU
 * through a FIFO. That works for an SD card and a touchscreen and does not
 * work at all for a network controller, an audio port, or a camera - the rates
 * are wrong and the CPU would spend its life copying. Those are exactly the
 * three devices a video call needs, so this has to exist before any of them.
 *
 * COHERENT - dma_alloc_coherent()
 *   Uncached memory from the reserved pool. For descriptor rings and control
 *   structures: things the CPU and the device both poke at, constantly and in
 *   small pieces. Uncached because maintaining a cache by hand around every
 *   descriptor update is both slower than not caching it and impossible to get
 *   right forever.
 *
 * STREAMING - dma_map_single() / dma_unmap_single()
 *   Ordinary cached memory, made safe around one transfer. For payload: packet
 *   contents, audio periods, video frames. Cached because the CPU reads and
 *   writes this data in bulk and the cache is worth having; maintained at the
 *   two edges of the transfer instead of avoided.
 *
 * THE DIRECTION IS NOT A HINT. It selects which cache operation runs, and the
 * wrong one fails silently:
 *
 *   DMA_TO_DEVICE      CPU wrote it, device will read it   -> clean
 *   DMA_FROM_DEVICE    device wrote it, CPU will read it   -> invalidate
 *   DMA_BIDIRECTIONAL  both                                -> clean + invalidate
 *
 * Getting FROM_DEVICE wrong is the memorable one: the transfer succeeds, the
 * device really did write DRAM, and the CPU reads its own stale cache lines
 * instead - so the data is sometimes right, depending on what else touched
 * that address recently. Nothing faults and there is nothing in the log.
 */
#define DMA_TO_DEVICE		0
#define DMA_FROM_DEVICE		1
#define DMA_BIDIRECTIONAL	2

/**
 * dma_alloc_coherent() - uncached, physically contiguous memory
 * @size:       bytes; rounded up to a page
 * @dma_handle: filled in with the physical address the device must be given
 *
 * Return: kernel VA, or NULL. Needs no cache maintenance, ever.
 */
void *dma_alloc_coherent(unsigned long size, unsigned long *dma_handle);

/**
 * dma_free_coherent() - release memory from dma_alloc_coherent()
 * @va:   what dma_alloc_coherent() returned
 * @size: the same size it was asked for
 */
void dma_free_coherent(void *va, unsigned long size);

/**
 * dma_map_single() - hand a cached buffer to a device for one transfer
 * @va:   buffer start
 * @size: length in bytes
 * @dir:  DMA_TO_DEVICE / DMA_FROM_DEVICE / DMA_BIDIRECTIONAL
 *
 * Call BEFORE starting the transfer. The buffer must not be touched by the CPU
 * between this and dma_unmap_single() - that is the entire contract, and the
 * reason it is a pair of calls rather than one.
 *
 * Return: physical address for the device, or 0 if the buffer is not in the
 * direct map (a DMA controller addresses DRAM, so a VA the kernel cannot
 * translate to a physical address cannot be given to one).
 */
unsigned long dma_map_single(void *va, unsigned long size, int dir);

/**
 * dma_unmap_single() - take a buffer back after a transfer
 *
 * Call AFTER the device reports completion and before the CPU reads the data.
 * For DMA_FROM_DEVICE this is where the stale cache lines are dropped, so
 * skipping it does not merely leak bookkeeping - it means reading the wrong
 * bytes.
 */
void dma_unmap_single(void *va, unsigned long size, int dir);

/* How much of the coherent pool is gone — for boot logs and for explaining a
 * failed allocation, since the pool is a fixed size by design. */
unsigned long dma_pool_free_bytes(void);

void dma_init(void);

#endif /* _NOTHAN_DMA_H */
