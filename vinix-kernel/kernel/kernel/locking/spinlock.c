/* ============================================================
 * spinlock.c
 * ------------------------------------------------------------
 * Uniprocessor spinlock — IRQ-disable for exclusivity.
 * ============================================================ */

#include "spinlock.h"

void spin_lock(spinlock_t *l)
{
    /* Single-core: cannot be preempted by another task while IRQs
     * are disabled. The caller is expected to have IRQs off for
     * any lock it holds. Here we just mark the flag. */
    l->locked = 1;
    __asm__ __volatile__("dmb" ::: "memory");
}

void spin_unlock(spinlock_t *l)
{
    __asm__ __volatile__("dmb" ::: "memory");
    l->locked = 0;
}

uint32_t spin_lock_irqsave(spinlock_t *l)
{
    uint32_t flags;
    __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i"
                         : "=r"(flags) :: "memory");
    l->locked = 1;
    return flags;
}

void spin_unlock_irqrestore(spinlock_t *l, uint32_t cpsr_flags)
{
    l->locked = 0;
    __asm__ __volatile__("msr cpsr_c, %0"
                         :: "r"(cpsr_flags) : "memory", "cc");
}
