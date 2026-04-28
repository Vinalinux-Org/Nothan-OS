/* ============================================================
 * spinlock.h
 * ------------------------------------------------------------
 * ARMv7 spinlock + IRQ-safe variants.
 * ============================================================ */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

typedef struct {
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT  { .locked = 0 }

static inline void spin_lock_init(spinlock_t *l) { l->locked = 0; }

void     spin_lock(spinlock_t *l);
void     spin_unlock(spinlock_t *l);

/* IRQ-safe — disables IRQ across the critical section. Pair them. */
uint32_t spin_lock_irqsave(spinlock_t *l);
void     spin_unlock_irqrestore(spinlock_t *l, uint32_t cpsr_flags);

#endif
