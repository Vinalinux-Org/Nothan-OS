/* ============================================================
 * barrier.h
 * ------------------------------------------------------------
 * ARMv7 memory barrier + compiler-barrier wrappers.
 * ============================================================ */

#ifndef BARRIER_H
#define BARRIER_H

#define dmb()      __asm__ __volatile__("dmb" ::: "memory")
#define dsb()      __asm__ __volatile__("dsb" ::: "memory")
#define isb()      __asm__ __volatile__("isb" ::: "memory")
#define barrier()  __asm__ __volatile__("" ::: "memory")

#define smp_mb()   dmb()
#define smp_rmb()  dmb()
#define smp_wmb()  dmb()

#endif
