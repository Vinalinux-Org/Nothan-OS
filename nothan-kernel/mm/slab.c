/*
 * mm/slab.c - Slab allocator (kmalloc/kfree for fixed-size objects)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/slab.h>
#include <nothan/mm.h>
#include <nothan/printk.h>
#include <asm/memory.h>
#include <asm/irqflags.h>

/*
 * WHAT THE CRITICAL SECTIONS BELOW PROTECT
 *
 *   cache->free_list      singly-linked list of free objects, threaded through
 *                         the first word of each free object itself
 *   cache->free_objects   running count
 *   page->slab            set when a page is handed to a cache, read by kfree()
 *                         to decide slab-object vs whole-buddy-block
 *
 * The subtle part is not the list surgery - it is that kmalloc() does
 * CHECK -> REFILL -> ACT on cache->free_list with a call to alloc_pages() in
 * the middle:
 *
 *      if (!cache->free_list)      <- CHECK
 *              slab_fill_page(...) <- REFILL (allocates a page)
 *      obj = cache->free_list;     <- ACT
 *
 * Masking inside alloc_pages() alone would leave CHECK..ACT open: a preemption
 * after CHECK lets another task drain the very list we just found non-empty,
 * and we come back to read a NULL free_list and dereference it.  So the
 * boundary has to enclose the whole sequence, which means it NESTS over the
 * one inside alloc_pages().  That nesting is safe by construction:
 * local_irq_restore() puts back the flags that local_irq_save() captured, so
 * the inner section leaves IRQs masked - it never re-enables them under us.
 * (This is exactly the property the header comment in asm/irqflags.h calls
 * out, and exactly what an unconditional "cpsie i" would get wrong.)
 *
 * Cost accepted: IRQs stay masked across alloc_pages() + slab_fill_page() on a
 * refill.  That is bounded - a buddy walk plus one page of pointer writes, no
 * I/O and no printk - so it is short in the same way the buddy section is.
 *
 * On SMP each of these becomes a real cache->lock.
 */

#define SLAB_SIZES		7

/**
 * struct slab_cache - Manages a pool of fixed-size objects
 * @obj_size: Size of a single object in bytes
 * @objs_per_page: Number of objects that fit in a single page
 * @free_objects: Current number of free objects in this cache
 * @free_list: Pointer to the first available free object
 */
struct slab_cache {
	size_t obj_size;
	unsigned int objs_per_page;
	unsigned int free_objects;
	void *free_list;
};

static struct slab_cache caches[SLAB_SIZES];
static const size_t cache_sizes[SLAB_SIZES] = {
	32, 64, 128, 256, 512, 1024, 2048
};

/*
 * Fill a freshly-allocated page with linked free objects.
 * The first unsigned long of each free object stores the
 * pointer to the next free object.
 */
static void slab_fill_page(struct slab_cache *cache, struct page *page)
{
	unsigned long pa = page_to_phys(get_zone(), page);
	void *base = (void *)__phys_to_virt(pa);
	page->slab = cache;

	/*
	 * Build a singly-linked free list through the objects:
	 * obj[i]->next = obj[i+1]; obj[last]->next = old free_list head.
	 */
	for (unsigned int i = 0; i < cache->objs_per_page; i++) {
		void *obj  = base + i * cache->obj_size;
		void *next = (i + 1 < cache->objs_per_page)
			     ? base + (i + 1) * cache->obj_size
			     : cache->free_list;
		*(void **)obj = next;
	}

	cache->free_list = base;
	cache->free_objects += cache->objs_per_page;
}

/**
 * slab_init() - Initialize the slab allocator
 *
 * Sets up the caches for various object sizes and pre-allocates
 * one page for each cache.
 */
void slab_init(void)
{
	for (unsigned int i = 0; i < SLAB_SIZES; i++) {
		caches[i].obj_size = cache_sizes[i];
		caches[i].objs_per_page = 1u << (PAGE_SHIFT - (i + 5));
		caches[i].free_objects = 0;
		caches[i].free_list = NULL;

		struct page *page = alloc_pages(GFP_KERNEL, 0);
		if (page)
			slab_fill_page(&caches[i], page);
	}

	printk("[SLAB] kmalloc classes:");
	for (unsigned int i = 0; i < SLAB_SIZES; i++)
		printk(" %d", cache_sizes[i]);
	printk("\n");
}

/**
 * kmalloc() - Allocate memory from the slab allocator or buddy allocator
 * @size: Number of bytes to allocate
 * @flags: Allocation flags (e.g. GFP_KERNEL)
 *
 * Return: Pointer to the allocated memory, or NULL if out of memory.
 */
void *kmalloc(size_t size, unsigned int flags)
{
	unsigned long irqflags;

	(void)flags;

	/* Sizes larger than the biggest cache go to the buddy allocator. */
	if (size > cache_sizes[SLAB_SIZES - 1]) {
		unsigned int order = 0;
		while ((PAGE_SIZE << order) < size)
			order++;

		/* No slab state is touched on this path: alloc_pages() masks for
		 * its own free lists, and the page it returns is exclusively ours
		 * before it returns, so page->slab/private need no section. */
		struct page *page = alloc_pages(GFP_KERNEL, order);
		if (!page)
			return NULL;
		page->slab = NULL;
		/* Stash the order so kfree() can release the block by pointer
		 * alone. alloc_pages() leaves private=0 and clears PG_BUDDY on the
		 * head page, so this is free to reuse and won't read as a buddy. */
		page->private = order;
		return (void *)__phys_to_virt(page_to_phys(get_zone(), page));
	}

	/* Picking the cache reads only immutable table data - keep it out. */
	unsigned int idx = 0;
	for (; idx < SLAB_SIZES; idx++)
		if (cache_sizes[idx] >= size)
			break;

	struct slab_cache *cache = &caches[idx];

	/* CHECK -> REFILL -> ACT must be indivisible; see the file header. */
	local_irq_save(irqflags);

	if (!cache->free_list) {
		struct page *page = alloc_pages(GFP_KERNEL, 0);
		if (!page) {
			local_irq_restore(irqflags);
			return NULL;
		}
		slab_fill_page(cache, page);
	}

	void *obj = cache->free_list;
	cache->free_list = *(void **)obj;
	cache->free_objects--;

	local_irq_restore(irqflags);

	return obj;
}

/**
 * kfree() - Free memory previously allocated by kmalloc
 * @ptr: Pointer to the memory to free
 */
void kfree(void *ptr)
{
	unsigned long irqflags;

	if (!ptr)
		return;

	struct zone *zone = get_zone();

	/* Convert VA to PA, then to page. */
	unsigned long pa = __virt_to_phys((unsigned long)ptr);
	if (pa < zone->base_pa || pa >= zone->end_pa)
		return;

	unsigned long pfn = (pa - zone->base_pa) >> PAGE_SHIFT;
	struct page *page = pfn_to_page(zone, pfn);
	struct slab_cache *cache = page->slab;

	if (!cache) {
		/*
		 * Direct buddy allocation (size > largest cache). kmalloc() stashed
		 * the order in page->private; release the whole block so large
		 * allocations (e.g. a task's 16 KB kernel stack) don't leak.
		 *
		 * No slab section needed - __free_pages() masks for the buddy
		 * lists, and page->slab/private belong to a block only this
		 * caller still holds.
		 */
		__free_pages(page, page->private);
		return;
	}

	/* Push onto the free list: two stores that must not be split, or a
	 * preempting allocator sees a list head pointing at an object whose
	 * next-pointer is still stale. */
	local_irq_save(irqflags);

	void **head = (void **)ptr;
	*head = cache->free_list;
	cache->free_list = ptr;
	cache->free_objects++;

	local_irq_restore(irqflags);
}
