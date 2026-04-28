/* ============================================================
 * atomic.h
 * ------------------------------------------------------------
 * Atomic integer via ARMv7 LDREX/STREX.
 * ============================================================ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include "types.h"

typedef struct { volatile int32_t counter; } atomic_t;

#define ATOMIC_INIT(v)  { .counter = (v) }

static inline int32_t atomic_read(const atomic_t *v)      { return v->counter; }
static inline void    atomic_set(atomic_t *v, int32_t i)  { v->counter = i; }

int32_t atomic_add_return(atomic_t *v, int32_t delta);
int32_t atomic_cmpxchg(atomic_t *v, int32_t expected, int32_t new_val);

static inline int32_t atomic_inc_return(atomic_t *v) { return atomic_add_return(v,  1); }
static inline int32_t atomic_dec_return(atomic_t *v) { return atomic_add_return(v, -1); }
static inline void    atomic_inc(atomic_t *v)        { atomic_add_return(v,  1); }
static inline void    atomic_dec(atomic_t *v)        { atomic_add_return(v, -1); }

#endif
