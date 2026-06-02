#include <nothan/types.h>
#include <nothan/slab.h>
#include <nothan/mm.h>
#include <asm/memory.h>

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

	for (unsigned int i = 0; i < cache->objs_per_page; i++) {
		void *obj = base + i * cache->obj_size;
		void **next = (void **)obj;
		*next = cache->free_list;
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
	(void)flags;

	/* Sizes larger than the biggest cache go to the buddy allocator. */
	if (size > cache_sizes[SLAB_SIZES - 1]) {
		unsigned int order = 0;
		while ((PAGE_SIZE << order) < size)
			order++;
		struct page *page = alloc_pages(GFP_KERNEL, order);
		if (!page)
			return NULL;
		page->slab = NULL;
		return (void *)__phys_to_virt(page_to_phys(get_zone(), page));
	}

	unsigned int idx = 0;
	for (; idx < SLAB_SIZES; idx++)
		if (cache_sizes[idx] >= size)
			break;

	struct slab_cache *cache = &caches[idx];

	if (!cache->free_list) {
		struct page *page = alloc_pages(GFP_KERNEL, 0);
		if (!page)
			return NULL;
		slab_fill_page(cache, page);
	}

	void *obj = cache->free_list;
	cache->free_list = *(void **)obj;
	cache->free_objects--;

	return obj;
}

/**
 * kfree() - Free memory previously allocated by kmalloc
 * @ptr: Pointer to the memory to free
 */
void kfree(void *ptr)
{
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
		 * Direct buddy allocation — the caller must know
		 * the right order.  We cannot recover the order
		 * from the pointer alone, so this path is a no-op
		 * until we store the order in page metadata.
		 */
		return;
	}

	void **head = (void **)ptr;
	*head = cache->free_list;
	cache->free_list = ptr;
	cache->free_objects++;
}
