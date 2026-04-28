/* ============================================================
 * slab.h
 * ------------------------------------------------------------
 * Fixed size-class slab allocator + generic kmalloc/kfree.
 * ============================================================ */

#ifndef SLAB_H
#define SLAB_H

#include "types.h"
#include "page_alloc.h"

struct kmem_cache;

struct kmem_cache *kmem_cache_create(const char *name, uint32_t obj_size);
void  *kmem_cache_alloc(struct kmem_cache *cache, gfp_t gfp);
void   kmem_cache_free(struct kmem_cache *cache, void *obj);

void   slab_init(void);
void   slab_selftest(void);

void  *kmalloc(uint32_t size, gfp_t gfp);
void   kfree(void *ptr);

/* Stats — useful for future /proc/slabinfo. */
uint32_t slab_cache_free_count(struct kmem_cache *cache);
uint32_t slab_cache_total_count(struct kmem_cache *cache);

#endif
