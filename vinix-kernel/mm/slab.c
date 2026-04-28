/* ============================================================
 * slab.c
 * ------------------------------------------------------------
 * Fixed size-class slab + kmalloc/kfree.
 * ============================================================ */

#include "slab.h"
#include "mmu.h"
#include "page_alloc.h"
#include "assert.h"
#include "uart.h"
#include "types.h"

#define SLAB_MAX_CACHES      16
#define SLAB_PAGE_HDR_BYTES  8   /* keeps first obj 8-byte aligned */

/* kfree looks up the cache through this header at the start of each
 * slab page, so obj_size must leave room for at least obj-sized tail. */
struct slab_page_hdr {
    struct kmem_cache *cache;
    uint32_t           pad;
};

struct free_obj {
    struct free_obj *next;
};

struct kmem_cache {
    const char      *name;
    uint32_t         obj_size;
    uint32_t         objs_per_page;
    struct free_obj *freelist;
    uint32_t         total_objs;
    uint32_t         free_objs;
    int              in_use;
};

static struct kmem_cache cache_pool[SLAB_MAX_CACHES];

/* kmalloc size-class caches. 2048 is the largest: larger allocations
 * return NULL in MVP (caller should use alloc_pages directly). */
static const uint32_t kmalloc_sizes[] = {32, 64, 128, 256, 512, 1024, 2048};
#define KMALLOC_CLASSES  (sizeof(kmalloc_sizes) / sizeof(kmalloc_sizes[0]))
static struct kmem_cache *kmalloc_caches[KMALLOC_CLASSES];

static uint32_t align_up_8(uint32_t x) { return (x + 7u) & ~7u; }

static int slab_refill(struct kmem_cache *cache)
{
    uint32_t pa = alloc_pages(GFP_KERNEL, 0);
    if (pa == 0)
    {
        return -1;
    }

    uint8_t *page_va = (uint8_t *)(pa + VA_OFFSET);
    struct slab_page_hdr *hdr = (struct slab_page_hdr *)page_va;
    hdr->cache = cache;
    hdr->pad   = 0;

    uint8_t *obj_base = page_va + SLAB_PAGE_HDR_BYTES;
    for (uint32_t i = 0; i < cache->objs_per_page; i++)
    {
        struct free_obj *obj =
            (struct free_obj *)(obj_base + i * cache->obj_size);
        obj->next = cache->freelist;
        cache->freelist = obj;
    }

    cache->total_objs += cache->objs_per_page;
    cache->free_objs  += cache->objs_per_page;
    return 0;
}

struct kmem_cache *kmem_cache_create(const char *name, uint32_t obj_size)
{
    ASSERT(obj_size >= sizeof(struct free_obj));
    ASSERT(obj_size <= PAGE_SIZE - SLAB_PAGE_HDR_BYTES);

    uint32_t aligned = align_up_8(obj_size);

    for (uint32_t i = 0; i < SLAB_MAX_CACHES; i++)
    {
        if (!cache_pool[i].in_use)
        {
            struct kmem_cache *c = &cache_pool[i];
            c->name          = name;
            c->obj_size      = aligned;
            c->objs_per_page = (PAGE_SIZE - SLAB_PAGE_HDR_BYTES) / aligned;
            c->freelist      = NULL;
            c->total_objs    = 0;
            c->free_objs     = 0;
            c->in_use        = 1;
            return c;
        }
    }
    return NULL;
}

void *kmem_cache_alloc(struct kmem_cache *cache, gfp_t gfp)
{
    ASSERT(cache != NULL);
    ASSERT(cache->in_use);
    (void)gfp;

    if (cache->freelist == NULL)
    {
        if (slab_refill(cache) < 0)
        {
            return NULL;
        }
    }

    struct free_obj *obj = cache->freelist;
    cache->freelist = obj->next;
    cache->free_objs--;
    return obj;
}

void kmem_cache_free(struct kmem_cache *cache, void *ptr)
{
    ASSERT(cache != NULL);
    ASSERT(ptr != NULL);

    struct free_obj *obj = (struct free_obj *)ptr;
    obj->next = cache->freelist;
    cache->freelist = obj;
    cache->free_objs++;
}

void *kmalloc(uint32_t size, gfp_t gfp)
{
    if (size == 0)
    {
        return NULL;
    }
    for (uint32_t i = 0; i < KMALLOC_CLASSES; i++)
    {
        if (size <= kmalloc_sizes[i])
        {
            return kmem_cache_alloc(kmalloc_caches[i], gfp);
        }
    }
    return NULL;
}

void kfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    uint32_t page_va = (uint32_t)ptr & PAGE_MASK;
    struct slab_page_hdr *hdr = (struct slab_page_hdr *)page_va;
    ASSERT(hdr->cache != NULL);
    kmem_cache_free(hdr->cache, ptr);
}

uint32_t slab_cache_free_count(struct kmem_cache *cache)
{
    return cache->free_objs;
}

uint32_t slab_cache_total_count(struct kmem_cache *cache)
{
    return cache->total_objs;
}

void slab_init(void)
{
    for (uint32_t i = 0; i < KMALLOC_CLASSES; i++)
    {
        kmalloc_caches[i] = kmem_cache_create("kmalloc", kmalloc_sizes[i]);
        ASSERT(kmalloc_caches[i] != NULL);
    }
    pr_info("[SLAB] kmalloc classes 32..2048 ready\n");
}

/* ============================================================
 * Self-test
 * ============================================================ */

void slab_selftest(void)
{
    struct kmem_cache *c = kmem_cache_create("test64", 64);
    if (c == NULL)
    {
        pr_info("[SLAB] FAIL: kmem_cache_create returned NULL\n");
        return;
    }

    void *objs[10];
    for (int i = 0; i < 10; i++)
    {
        objs[i] = kmem_cache_alloc(c, GFP_KERNEL);
        if (objs[i] == NULL)
        {
            pr_info("[SLAB] FAIL: cache_alloc #%d NULL\n", i);
            return;
        }
        for (int j = 0; j < i; j++)
        {
            if (objs[i] == objs[j])
            {
                pr_info("[SLAB] FAIL: obj #%d dup of #%d (ptr 0x%x)\n",
                            i, j, (uint32_t)objs[i]);
                return;
            }
        }
    }

    for (int i = 0; i < 10; i++)
    {
        kmem_cache_free(c, objs[i]);
    }

    void *reuse = kmem_cache_alloc(c, GFP_KERNEL);
    int matched = 0;
    for (int i = 0; i < 10; i++)
    {
        if (reuse == objs[i])
        {
            matched = 1;
            break;
        }
    }
    if (!matched)
    {
        pr_info("[SLAB] FAIL: realloc did not reuse freed object\n");
        return;
    }
    kmem_cache_free(c, reuse);

    void *a = kmalloc(16, GFP_KERNEL);
    void *b = kmalloc(100, GFP_KERNEL);
    void *d = kmalloc(1000, GFP_KERNEL);
    void *big = kmalloc(3000, GFP_KERNEL);

    if (a == NULL || b == NULL || d == NULL)
    {
        pr_info("[SLAB] FAIL: kmalloc returned NULL for supported size\n");
        return;
    }
    if (big != NULL)
    {
        pr_info("[SLAB] FAIL: kmalloc(3000) should return NULL\n");
        kfree(big);
        return;
    }
    kfree(a);
    kfree(b);
    kfree(d);
}
