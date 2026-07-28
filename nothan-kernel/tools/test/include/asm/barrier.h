#ifndef _NOTHAN_BARRIER_H
#define _NOTHAN_BARRIER_H
/*
 * tools/test/include/asm/barrier.h - host stub.
 *
 * Only the memory barriers are stubbed. The CP15 accessors and cache/TLB
 * maintenance in arch/arm/include/asm/barrier.h are DELIBERATELY absent: host
 * code has no business touching them, and a missing-symbol error is a better
 * outcome than a stub that silently pretends the operation happened.
 *
 * On x86 the hardware is strongly ordered, so a compiler barrier is the
 * correct lowering for all of these. It stops GCC reordering across the point,
 * which is the part that matters for the wait/wake logic being tested.
 */

#define dsb()	__asm__ __volatile__ ("" : : : "memory")
#define dmb()	__asm__ __volatile__ ("" : : : "memory")
#define isb()	__asm__ __volatile__ ("" : : : "memory")

#endif /* _NOTHAN_BARRIER_H */
