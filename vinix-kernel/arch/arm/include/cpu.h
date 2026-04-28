/* ============================================================
 * cpu.h
 * ------------------------------------------------------------
 * ARM CPU control interface
 * ============================================================ */

#ifndef CPU_H
#define CPU_H

#include "types.h"

/* ============================================================
 * CPSR Bit Definitions
 * ============================================================ */

#define CPSR_MODE_MASK  0x1F    /* Mode bits [4:0] */
#define CPSR_MODE_USR   0x10    /* User mode */
#define CPSR_MODE_FIQ   0x11    /* FIQ mode */
#define CPSR_MODE_IRQ   0x12    /* IRQ mode */
#define CPSR_MODE_SVC   0x13    /* Supervisor mode */
#define CPSR_MODE_ABT   0x17    /* Abort mode */
#define CPSR_MODE_UND   0x1B    /* Undefined mode */
#define CPSR_MODE_SYS   0x1F    /* System mode */

#define CPSR_FIQ_BIT    (1 << 6)    /* F bit - FIQ mask */
#define CPSR_IRQ_BIT    (1 << 7)    /* I bit - IRQ mask */

/* ============================================================
 * IRQ Control Functions
 * ============================================================ */

/* Precondition: intc_init() + irq_init() done, with at least one
 * handler registered (or willing to absorb spurious IRQs). */
static inline void irq_enable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"
        "bic %0, %0, %1\n"
        "msr cpsr_c, %0"
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

static inline void irq_disable(void)
{
    uint32_t cpsr;
    asm volatile(
        "mrs %0, cpsr\n"
        "orr %0, %0, %1\n"
        "msr cpsr_c, %0"
        : "=r"(cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
}

static inline int irq_is_enabled(void)
{
    uint32_t cpsr;
    asm volatile("mrs %0, cpsr" : "=r"(cpsr));
    return !(cpsr & CPSR_IRQ_BIT);
}

/* Returns prior CPSR — feed it back into irq_restore(). */
static inline uint32_t irq_save(void)
{
    uint32_t cpsr, new_cpsr;
    asm volatile(
        "mrs %0, cpsr\n"
        "orr %1, %0, %2\n"
        "msr cpsr_c, %1"
        : "=r"(cpsr), "=r"(new_cpsr)
        : "I"(CPSR_IRQ_BIT)
        : "memory"
    );
    return cpsr;
}

static inline void irq_restore(uint32_t flags)
{
    asm volatile("msr cpsr_c, %0" : : "r"(flags) : "memory");
}

/* ============================================================
 * Memory Barrier Functions
 * ============================================================ */

static inline void dsb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory");
}

static inline void dmb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
}

static inline void isb(void)
{
    asm volatile("mcr p15, 0, %0, c7, c5, 4" : : "r"(0) : "memory");
}

static inline void wfi(void)
{
    asm volatile("wfi" : : : "memory");
}

#endif /* CPU_H */