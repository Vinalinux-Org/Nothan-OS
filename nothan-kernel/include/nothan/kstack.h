#ifndef _NOTHAN_KSTACK_H
#define _NOTHAN_KSTACK_H

#include <nothan/mm.h>

/*
 * Kernel stacks, each with an unmapped guard page below it.
 *
 * WHY THIS IS NOT kmalloc()
 *
 * Kernel stacks used to come from kmalloc(), which puts them in the buddy pool
 * inside the 0xC0000000 direct map. That map is built from 1 MB SECTIONS, and a
 * section has no per-page descriptors - so there is no way to leave a hole
 * below a stack. A stack that overruns simply writes into whatever kmalloc
 * block sits underneath it: an L2 page table, another task's task_struct, a
 * struct file.
 *
 * Nothing detects that. The write succeeds, the kernel carries on, and the
 * damage surfaces later - in a different subsystem, at a different time, with
 * a fault address that points at the victim and says nothing about the culprit.
 * That is precisely the bug class a UART cannot investigate, and the one
 * Documentation/design-philosophy.md says to remove at design time rather than
 * be careful around.
 *
 * So kernel stacks get their own VA window, mapped page by page through real L2
 * tables, with the pages below each stack left unmapped. An overflow then takes
 * a translation fault ON THE INSTRUCTION THAT CAUSES IT: DFAR names the address
 * just below the stack, PC names the code that pushed too far, and the existing
 * abort handler prints both. The bug reports itself.
 *
 * The physical pages still come from the buddy allocator and are still visible
 * through the direct map as well. Both mappings are Normal write-back
 * write-allocate, so the two views agree - this is aliasing, not a mismatched
 * memory attribute.
 *
 * WHAT IT COSTS
 *
 * One page-table entry per stack page, plus a slot's worth of address space per
 * stack that is deliberately never mapped. Address space is the cheap resource
 * here: the window costs no RAM beyond its L2 tables.
 */

/* 16 KB of usable stack — enough for a syscall frame plus a nested IRQ frame
 * plus a driver ISR plus printk's 256-byte line buffer, with room left. */
#define KSTACK_ORDER	2
#define KSTACK_PAGES	(1u << KSTACK_ORDER)
#define KSTACK_SIZE	(KSTACK_PAGES * PAGE_SIZE)

/**
 * kstack_init() - install the kernel-stack window into the master page table
 *
 * MUST run before the first task is created, and before any process page table
 * exists. pgd_alloc() copies the kernel half of the master table at the moment
 * a process is born, so an L1 entry added afterwards is invisible to every
 * process created before it. Kernel threads make that fatal rather than
 * merely wrong: __schedule() skips mmu_switch_mm() for a task with no mm, so a
 * kernel thread runs on whatever TTBR0 the previous task left installed - and
 * would fault the instant it touched its own stack through a process page
 * table that never learnt the window existed.
 *
 * Installing all the L1 entries up front, before anything can copy them, is
 * what makes the window global by construction. Only the leaf entries change
 * afterwards, and those live in L2 tables that every process already points at.
 */
void kstack_init(void);

/**
 * kstack_alloc() - allocate a guarded kernel stack
 *
 * Return: the LOWEST address of the usable stack, or NULL. The initial stack
 * pointer is that address + KSTACK_SIZE; the KSTACK_SIZE bytes below it are
 * the guard and are not mapped.
 */
void *kstack_alloc(void);

/**
 * kstack_free() - release a stack obtained from kstack_alloc()
 * @base: exactly what kstack_alloc() returned
 */
void kstack_free(void *base);

/* How many stacks are live, and the ceiling — for boot logs and for saying
 * something useful when task creation starts failing. */
unsigned int kstack_in_use(void);
unsigned int kstack_capacity(void);

/*
 * Canaries for the stacks the linker lays out - boot/SVC (which the idle task
 * inherits for good) and the exception-mode stacks. Those exist before the MMU
 * and the page allocator do, so they cannot live in the guarded window; a
 * canary is the strongest thing available to them.
 *
 * stack_guard_check() runs from the timer tick, so an overflow is reported
 * within one tick of happening rather than never.
 */
void stack_guard_init(void);
void stack_guard_check(void);

#endif /* _NOTHAN_KSTACK_H */
