/* ============================================================
 * vmm.h
 * ------------------------------------------------------------
 * Per-process address space descriptor + VMA list.
 * ============================================================ */

#ifndef VMM_H
#define VMM_H

#include "types.h"

#define VM_READ    0x1
#define VM_WRITE   0x2
#define VM_EXEC    0x4
#define VM_USER    0x8

struct vm_area_struct {
    uint32_t vm_start;
    uint32_t vm_end;
    uint32_t vm_flags;
    struct vm_area_struct *vm_next;
};

struct mm_struct {
    struct vm_area_struct *mmap;
    uint32_t pgd_pa;
    int      in_use;
};

void vmm_init(void);
void vmm_selftest(void);

struct mm_struct *mm_alloc(void);
void mm_free(struct mm_struct *mm);

/* vma_create inserts sorted by vm_start. Caller owns the address range. */
struct vm_area_struct *vma_create(struct mm_struct *mm,
                                   uint32_t start, uint32_t end,
                                   uint32_t flags);

#endif
