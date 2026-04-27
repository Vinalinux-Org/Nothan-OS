/* ============================================================
 * vmm.c
 * ------------------------------------------------------------
 * Per-process mm_struct + VMA list.
 * ============================================================ */

#include "vmm.h"
#include "slab.h"
#include "mmu.h"
#include "page_alloc.h"
#include "assert.h"
#include "uart.h"
#include "types.h"

static struct kmem_cache *mm_cache;
static struct kmem_cache *vma_cache;

void vmm_init(void)
{
    mm_cache  = kmem_cache_create("mm_struct", sizeof(struct mm_struct));
    vma_cache = kmem_cache_create("vm_area",   sizeof(struct vm_area_struct));
    ASSERT(mm_cache  != NULL);
    ASSERT(vma_cache != NULL);

    pr_info("[VMM] caches ready (mm_struct=%dB, vm_area=%dB)\n",
                sizeof(struct mm_struct), sizeof(struct vm_area_struct));
}

struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm = kmem_cache_alloc(mm_cache, GFP_KERNEL);
    if (mm == NULL)
    {
        return NULL;
    }

    uint32_t pgd_pa = mmu_new_pgd();
    if (pgd_pa == 0)
    {
        kmem_cache_free(mm_cache, mm);
        return NULL;
    }

    mm->mmap   = NULL;
    mm->pgd_pa = pgd_pa;
    mm->in_use = 1;
    return mm;
}

void mm_free(struct mm_struct *mm)
{
    ASSERT(mm != NULL);
    ASSERT(mm->in_use);

    struct vm_area_struct *vma = mm->mmap;
    while (vma != NULL)
    {
        struct vm_area_struct *next = vma->vm_next;
        kmem_cache_free(vma_cache, vma);
        vma = next;
    }

    mmu_free_pgd(mm->pgd_pa);
    mm->in_use = 0;
    kmem_cache_free(mm_cache, mm);
}

struct vm_area_struct *vma_create(struct mm_struct *mm,
                                   uint32_t start, uint32_t end,
                                   uint32_t flags)
{
    ASSERT(mm != NULL);
    ASSERT(mm->in_use);
    ASSERT(start < end);

    struct vm_area_struct *vma = kmem_cache_alloc(vma_cache, GFP_KERNEL);
    if (vma == NULL)
    {
        return NULL;
    }

    vma->vm_start = start;
    vma->vm_end   = end;
    vma->vm_flags = flags;

    struct vm_area_struct **pp = &mm->mmap;
    while (*pp != NULL && (*pp)->vm_start < start)
    {
        pp = &(*pp)->vm_next;
    }
    vma->vm_next = *pp;
    *pp = vma;

    return vma;
}

/* ============================================================
 * Self-test
 * ============================================================ */

void vmm_selftest(void)
{
    /* Warm up mm/vma caches so the first cycle's slab refill doesn't
     * skew the leak snapshot. */
    struct mm_struct *warm = mm_alloc();
    ASSERT(warm != NULL);
    vma_create(warm, 0x40000000, 0x40001000, VM_READ);
    mm_free(warm);

    uint32_t pages_before = page_alloc_free_pages();

    for (int i = 0; i < 5; i++)
    {
        struct mm_struct *mm = mm_alloc();
        if (mm == NULL)
        {
            pr_info("[VMM] FAIL: mm_alloc #%d returned NULL\n", i);
            return;
        }
        if (mm->pgd_pa == 0)
        {
            pr_info("[VMM] FAIL: mm_alloc #%d pgd_pa=0\n", i);
            return;
        }
        if ((mm->pgd_pa & 0x3FFFu) != 0)
        {
            pr_info("[VMM] FAIL: pgd_pa 0x%x not 16KB aligned\n", mm->pgd_pa);
            return;
        }
        vma_create(mm, 0x40000000 + i * 0x1000, 0x40001000 + i * 0x1000,
                   VM_READ | VM_USER);
        mm_free(mm);
    }

    if (page_alloc_free_pages() != pages_before)
    {
        pr_info("[VMM] FAIL: page leak — before=%d after=%d\n",
                    pages_before, page_alloc_free_pages());
        return;
    }

    struct mm_struct *mm = mm_alloc();
    vma_create(mm, 0x40000000, 0x40001000, VM_READ);
    vma_create(mm, 0x40010000, 0x40011000, VM_WRITE);
    vma_create(mm, 0x40005000, 0x40006000, VM_EXEC);

    uint32_t prev = 0;
    int count = 0;
    struct vm_area_struct *v = mm->mmap;
    while (v != NULL)
    {
        if (v->vm_start < prev)
        {
            pr_info("[VMM] FAIL: VMAs not sorted (%x < %x)\n",
                        v->vm_start, prev);
            mm_free(mm);
            return;
        }
        prev = v->vm_start;
        count++;
        v = v->vm_next;
    }
    if (count != 3)
    {
        pr_info("[VMM] FAIL: VMA count %d != 3\n", count);
        mm_free(mm);
        return;
    }
    mm_free(mm);
}
