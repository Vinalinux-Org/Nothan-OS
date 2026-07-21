#ifndef _NOTHAN_IRQFLAGS_H
#define _NOTHAN_IRQFLAGS_H

/*
 * arch/arm/include/asm/irqflags.h - local IRQ masking primitives
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Single-core AM335x (Cortex-A8): there is no cross-CPU contention, so a
 * "lock" here degenerates to masking IRQs on the local core.  These wrap the
 * open-coded "cpsid i" / "cpsie i" scattered around the scheduler so that a
 * critical section can be entered and left WITHOUT clobbering the caller's
 * prior IRQ state.
 *
 * Why save/restore instead of a bare cpsie: a caller that already runs with
 * IRQs masked (e.g. the IRQ path, or a nested critical section) must stay
 * masked on exit.  A blind "cpsie i" would re-enable IRQs underneath it and
 * open a window for re-entrancy.  local_irq_restore() puts back exactly what
 * local_irq_save() captured.
 */

/**
 * arch_local_irq_save - mask IRQs, returning the previous CPSR
 *
 * Returns the full CPSR captured before masking.  Pass it to
 * arch_local_irq_restore() to undo.
 */
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	__asm__ __volatile__(
		"mrs	%0, cpsr\n"	/* capture current state */
		"cpsid	i"		/* mask IRQs */
		: "=r" (flags)
		:
		: "memory", "cc");

	return flags;
}

/**
 * arch_local_irq_restore - restore the CPSR control byte captured earlier
 * @flags: value returned by arch_local_irq_save()
 *
 * Writes back the control field (mode + I/F/T bits).  Restoring in the same
 * processor mode makes the mode bits a no-op; the meaningful effect is
 * putting the I-bit back to its prior value.
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	__asm__ __volatile__(
		"msr	cpsr_c, %0"
		:
		: "r" (flags)
		: "memory", "cc");
}

#define local_irq_save(flags)					\
	do { (flags) = arch_local_irq_save(); } while (0)

#define local_irq_restore(flags)				\
	arch_local_irq_restore(flags)

#endif /* _NOTHAN_IRQFLAGS_H */
