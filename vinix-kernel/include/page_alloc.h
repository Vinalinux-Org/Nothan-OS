/* ============================================================
 * page_alloc.h
 * ------------------------------------------------------------
 * Bitmap page allocator — 4 KB granularity, Linux-style API.
 * ============================================================ */

#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include "types.h"

typedef uint32_t gfp_t;

#define GFP_KERNEL  0x01
#define GFP_ATOMIC  0x02

#define PAGE_SHIFT  12
#define PAGE_SIZE   (1u << PAGE_SHIFT)
#define PAGE_MASK   (~(PAGE_SIZE - 1u))

/* Order N = 2^N contiguous pages. Up to 1 MB so fork() can allocate a
 * user address space in one call (256 pages, naturally aligned). */
#define PAGE_MAX_ORDER  8

void page_alloc_init(void);
void page_alloc_selftest(void);

/* Returns physical address, or 0 on failure. */
uint32_t alloc_pages(gfp_t gfp, unsigned int order);
void     free_pages(uint32_t pa, unsigned int order);

static inline uint32_t __get_free_page(gfp_t gfp)
{
    return alloc_pages(gfp, 0);
}

static inline void free_page(uint32_t pa)
{
    free_pages(pa, 0);
}

uint32_t page_alloc_total_pages(void);
uint32_t page_alloc_free_pages(void);

#endif
