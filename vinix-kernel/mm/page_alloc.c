/* ============================================================
 * page_alloc.c
 * ------------------------------------------------------------
 * Bitmap page allocator.
 * ============================================================ */

#include "page_alloc.h"
#include "mach/memory.h"
#include "assert.h"
#include "uart.h"
#include "types.h"

/* Pool starts 16 MB into DDR to leave room for kernel image,
 * user bootstrap, and framebuffer. Ends at DDR end (128 MB). */
#define POOL_PA_BASE    0x81000000
#define POOL_PA_END     (PLATFORM_DDR_PA_BASE + (PLATFORM_DDR_SIZE_MB * 1024u * 1024u))
#define POOL_SIZE       (POOL_PA_END - POOL_PA_BASE)
#define POOL_PAGES      (POOL_SIZE / PAGE_SIZE)
#define BITMAP_WORDS    ((POOL_PAGES + 31u) / 32u)

static uint32_t bitmap[BITMAP_WORDS];
static uint32_t free_count;

static inline int bit_get(uint32_t idx)
{
    return (bitmap[idx >> 5] >> (idx & 31u)) & 1u;
}

static inline void bit_set(uint32_t idx)
{
    bitmap[idx >> 5] |= (1u << (idx & 31u));
}

static inline void bit_clear(uint32_t idx)
{
    bitmap[idx >> 5] &= ~(1u << (idx & 31u));
}

void page_alloc_init(void)
{
    for (uint32_t i = 0; i < BITMAP_WORDS; i++)
    {
        bitmap[i] = 0;
    }
    free_count = POOL_PAGES;

    pr_info("[PAGE] Pool: %d MB @ PA 0x%x (%d pages, bitmap %dB)\n",
                POOL_SIZE / (1024u * 1024u), POOL_PA_BASE,
                POOL_PAGES, BITMAP_WORDS * 4u);
}

uint32_t alloc_pages(gfp_t gfp, unsigned int order)
{
    (void)gfp;

    if (order > PAGE_MAX_ORDER)
    {
        return 0;
    }

    uint32_t count = 1u << order;

    /* Natural alignment: step by 'count' so returned PA is
     * aligned to (count * PAGE_SIZE). */
    for (uint32_t i = 0; i + count <= POOL_PAGES; i += count)
    {
        int all_zero = 1;
        for (uint32_t j = 0; j < count; j++)
        {
            if (bit_get(i + j))
            {
                all_zero = 0;
                break;
            }
        }
        if (all_zero)
        {
            for (uint32_t j = 0; j < count; j++)
            {
                bit_set(i + j);
            }
            free_count -= count;
            return POOL_PA_BASE + (i * PAGE_SIZE);
        }
    }
    return 0;
}

void free_pages(uint32_t pa, unsigned int order)
{
    ASSERT(pa >= POOL_PA_BASE);
    ASSERT(pa < POOL_PA_END);
    ASSERT((pa & (PAGE_SIZE - 1u)) == 0);
    ASSERT(order <= PAGE_MAX_ORDER);

    uint32_t count = 1u << order;
    uint32_t idx   = (pa - POOL_PA_BASE) / PAGE_SIZE;

    for (uint32_t j = 0; j < count; j++)
    {
        /* Catches double-free. */
        ASSERT(bit_get(idx + j) == 1);
        bit_clear(idx + j);
    }
    free_count += count;
}

uint32_t page_alloc_total_pages(void) { return POOL_PAGES; }
uint32_t page_alloc_free_pages(void)  { return free_count; }

/* ============================================================
 * Self-test
 * ============================================================ */

#define SELFTEST_ORDER0_N   100
#define SELFTEST_ORDER3_MASK ((1u << (PAGE_SHIFT + PAGE_MAX_ORDER)) - 1u)

void page_alloc_selftest(void)
{
    uint32_t snapshot = free_count;
    uint32_t pages[SELFTEST_ORDER0_N];

    for (int i = 0; i < SELFTEST_ORDER0_N; i++)
    {
        pages[i] = alloc_pages(GFP_KERNEL, 0);
        if (pages[i] == 0)
        {
            pr_info("[PAGE] FAIL: alloc #%d returned 0\n", i);
            return;
        }
        if ((pages[i] & (PAGE_SIZE - 1u)) != 0)
        {
            pr_info("[PAGE] FAIL: alloc #%d PA 0x%x not page-aligned\n",
                        i, pages[i]);
            return;
        }
        if (pages[i] < POOL_PA_BASE || pages[i] >= POOL_PA_END)
        {
            pr_info("[PAGE] FAIL: alloc #%d PA 0x%x out of pool\n",
                        i, pages[i]);
            return;
        }
        for (int j = 0; j < i; j++)
        {
            if (pages[i] == pages[j])
            {
                pr_info("[PAGE] FAIL: alloc #%d duplicate of #%d (PA 0x%x)\n",
                            i, j, pages[i]);
                return;
            }
        }
    }

    for (int i = 0; i < SELFTEST_ORDER0_N; i++)
    {
        free_page(pages[i]);
    }
    if (free_count != snapshot)
    {
        pr_info("[PAGE] FAIL: free_count %d != snapshot %d after order-0 free\n",
                    free_count, snapshot);
        return;
    }

    uint32_t big = alloc_pages(GFP_KERNEL, PAGE_MAX_ORDER);
    if (big == 0)
    {
        pr_info("[PAGE] FAIL: alloc order-%d returned 0\n", PAGE_MAX_ORDER);
        return;
    }
    if ((big & SELFTEST_ORDER3_MASK) != 0)
    {
        pr_info("[PAGE] FAIL: order-%d PA 0x%x not %d-byte aligned\n",
                    PAGE_MAX_ORDER, big, SELFTEST_ORDER3_MASK + 1u);
        return;
    }
    free_pages(big, PAGE_MAX_ORDER);
    if (free_count != snapshot)
    {
        pr_info("[PAGE] FAIL: free_count mismatch after order-%d free\n",
                    PAGE_MAX_ORDER);
        return;
    }
}
