#ifndef _NOTHAN_BARRIER_H
#define _NOTHAN_BARRIER_H
/*
 * Host stand-in for the ARM barriers.
 *
 * The compiler barrier is kept because the ring's correctness argument is
 * partly about ordering the compiler may not change; the processor half of it
 * cannot be tested here at all, and this file does not pretend otherwise.
 * What runs on the host is the protocol logic, not the memory model.
 */
#define dsb()	__asm__ __volatile__ ("" : : : "memory")
#define dmb()	__asm__ __volatile__ ("" : : : "memory")
#define isb()	__asm__ __volatile__ ("" : : : "memory")
#endif
