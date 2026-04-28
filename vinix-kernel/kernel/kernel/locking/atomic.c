/* ============================================================
 * atomic.c
 * ------------------------------------------------------------
 * Uniprocessor atomics via IRQ-disable.
 * ============================================================ */

#include "atomic.h"

static inline uint32_t irq_save_disable(void)
{
    uint32_t flags;
    __asm__ __volatile__("mrs %0, cpsr\n\tcpsid i"
                         : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags)
{
    __asm__ __volatile__("msr cpsr_c, %0" :: "r"(flags) : "memory", "cc");
}

int32_t atomic_add_return(atomic_t *v, int32_t delta)
{
    uint32_t flags = irq_save_disable();
    int32_t result = v->counter + delta;
    v->counter = result;
    irq_restore(flags);
    return result;
}

int32_t atomic_cmpxchg(atomic_t *v, int32_t expected, int32_t new_val)
{
    uint32_t flags = irq_save_disable();
    int32_t old = v->counter;
    if (old == expected)
    {
        v->counter = new_val;
    }
    irq_restore(flags);
    return old;
}
