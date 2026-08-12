#ifndef _ASM_IRQFLAGS_H
#define _ASM_IRQFLAGS_H

/*
 * arch/arm/include/asm/irqflags.h - the one critical-section primitive
 *
 * NothanOS is single-core, so masking interrupts is sufficient against every
 * race there is: the scheduler only switches tasks from the tick interrupt or
 * from an explicit schedule() call, so a region with interrupts masked cannot
 * be interrupted by another task either.  One mechanism covers task-vs-task
 * and task-vs-ISR alike.
 *
 * That is deliberate, not lazy.  A design with two primitives — preempt
 * disable for data shared between tasks, interrupt masking for data an ISR
 * also touches — requires classifying every shared structure correctly, and a
 * wrong classification is a silent race: the exact bug class
 * Documentation/design-philosophy.md §1 says a log-only system must eliminate
 * by construction.  With a single primitive there is no classification to get
 * wrong.  (§2 says the same thing from the other direction: one canonical
 * mechanism per subsystem.)
 *
 * Spin locks are not provided.  On a single core they degenerate into exactly
 * this, and the abstraction only earns its keep on SMP, which is not the
 * baseline for any of the three hardware tracks.
 *
 * The cost is interrupt latency, and it is worth stating in numbers: the
 * critical sections in this kernel are list operations, tens to a few hundred
 * cycles, comfortably under a microsecond at 1 GHz — against a 10 ms audio
 * deadline.  The danger is not masking, it is masking for a *long* time.  So:
 *
 *     Critical sections must be short and bounded.  No unbounded loops, no
 *     waiting on hardware, no printk inside one.
 *
 * ALWAYS use save/restore, never the bare disable/enable pair, unless the code
 * genuinely owns the global state (early boot, the halt path).  Bare enable
 * forces interrupts on regardless of what the caller had, which silently ends
 * an outer critical section — this kernel shipped that bug in six places
 * before this header existed.
 */

#include <nothan/types.h>

#define PSR_I_BIT	0x80

/**
 * local_irq_save() - mask interrupts, returning the previous state
 *
 * Pair with local_irq_restore().  Nests correctly: an inner region restores
 * to "still masked" rather than to "enabled".
 */
static inline unsigned long local_irq_save(void)
{
	unsigned long flags;

	__asm__ __volatile__(
		"mrs	%0, cpsr\n"
		"cpsid	i"
		: "=r" (flags) : : "memory", "cc");

	return flags;
}

/**
 * local_irq_restore() - put the interrupt mask back the way it was
 * @flags: the value returned by the matching local_irq_save()
 */
static inline void local_irq_restore(unsigned long flags)
{
	__asm__ __volatile__(
		"msr	cpsr_c, %0"
		: : "r" (flags) : "memory", "cc");
}

/*
 * Unconditional forms.  Only for code that owns the global interrupt state
 * outright: early boot before any task exists, the idle loop, and the halt
 * path.  Everywhere else wants save/restore.
 */
static inline void local_irq_disable(void)
{
	__asm__ __volatile__("cpsid i" : : : "memory", "cc");
}

static inline void local_irq_enable(void)
{
	__asm__ __volatile__("cpsie i" : : : "memory", "cc");
}

/** irqs_disabled() - true if interrupts are currently masked */
static inline int irqs_disabled(void)
{
	unsigned long flags;

	__asm__ __volatile__("mrs %0, cpsr" : "=r" (flags));

	return (flags & PSR_I_BIT) != 0;
}

#endif /* _ASM_IRQFLAGS_H */
