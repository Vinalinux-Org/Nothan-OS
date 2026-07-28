#ifndef _NOTHAN_IRQFLAGS_H
#define _NOTHAN_IRQFLAGS_H
/*
 * tools/test/include/asm/irqflags.h - host model of "IRQs masked".
 *
 * Mirrors the guard name of arch/arm/include/asm/irqflags.h on purpose: the
 * two headers are alternatives, never both in one translation unit.
 *
 * This is NO LONGER a no-op (it was, until the A7 harness landed).  On the
 * board, masking IRQs buys mutual exclusion against every other runnable
 * context on the core - preemption and ISRs both.  A host harness that mapped
 * it to nothing would let two pthreads sit inside one critical section at
 * once: it would model a machine NothanOS is not, and every failure it
 * reported would be a lie.
 *
 * So on the host, "IRQs masked" is one global lock:
 *
 *      local_irq_save()      take the big lock (only on the outermost entry)
 *      local_irq_restore()   drop it (only on the outermost exit)
 *
 * Nesting is counted per thread, so an inner restore must NOT release
 * anything - exactly how the ARM version behaves by putting back the saved
 * CPSR rather than blindly enabling.
 *
 * Consequence worth stating: this deliberately makes the host LESS concurrent
 * than raw pthreads.  The harness exists to find LOGIC bugs that survive on a
 * single core - lost wakeups, dropped or duplicated messages, torn ring
 * indices - NOT to hunt data races this kernel cannot experience today.  A
 * "more parallel" host would just report failures the board cannot have.
 *
 * Implementation: tools/test/host_sched.c
 */

unsigned long host_irq_save(void);
void	      host_irq_restore(unsigned long flags);

#define local_irq_save(flags)					\
	do { (flags) = host_irq_save(); } while (0)

#define local_irq_restore(flags)				\
	host_irq_restore(flags)

#endif /* _NOTHAN_IRQFLAGS_H */
