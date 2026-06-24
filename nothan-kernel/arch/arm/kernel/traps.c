/*
 * arch/arm/kernel/traps.c - ARM exception handlers (abort, undefined, FIQ)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/sched.h>

#define SPSR_MODE_MASK	0x1F
#define MODE_USER	0x10	/* ARM user mode */

/*
 * If the exception originated in user mode, kill the faulting task and
 * reschedule. Only halt if the fault came from kernel mode — kernel
 * exceptions are unrecoverable.
 */
static void handle_user_or_panic(unsigned int spsr, const char *tag)
{
	if ((spsr & SPSR_MODE_MASK) == MODE_USER) {
		printk("  [%s] killing user task \"%s\" pid=%d\n",
		       tag, runqueue.curr->comm, runqueue.curr->pid);
		do_exit(-1);
		/* NOTREACHED */
	}
	printk("  [%s] kernel-mode fault — halting\n", tag);
	while (1)
		;
}

/**
 * und_handler - handle undefined instruction exception
 * @spsr: saved program status register at fault
 */
void und_handler(unsigned int spsr)
{
	printk("\nException: Undefined Instruction!\n");
	printk("  SPSR=0x%08x\n", spsr);
	handle_user_or_panic(spsr, "UND");
}

/**
 * pabt_handler - handle prefetch abort
 * @spsr: saved program status register at fault
 *
 * Reads IFAR (Instruction Fault Address) and IFSR (Instruction Fault
 * Status) from CP15 to determine the fault address and reason.
 */
void pabt_handler(unsigned int spsr, unsigned int lr_usr)
{
	unsigned int ifar, ifsr;
	__asm__ __volatile__(
		"mrc p15, 0, %0, c6, c0, 2\n"	/* IFAR */
		"mrc p15, 0, %1, c5, c0, 1\n"	/* IFSR */
		: "=r"(ifar), "=r"(ifsr) : : "memory");

	printk("\nException: Prefetch Abort!\n");
	printk("  IFAR=0x%08x, IFSR=0x%08x\n", ifar, ifsr);
	printk("  LR_usr=0x%08x  SPSR=0x%08x\n", lr_usr, spsr);

	const char *reason;
	switch (ifsr & 0xF) {
	case 1:  reason = "Alignment fault"; break;
	case 4:  reason = "ICache maintenance fault"; break;
	case 7:  reason = "Translation fault (page/section)"; break;
	case 9:  reason = "Domain fault (section)"; break;
	case 10: reason = "Domain fault (page)"; break;
	case 13: reason = "Permission fault (section)"; break;
	case 15: reason = "Permission fault (page)"; break;
	default: reason = "Unknown"; break;
	}
	printk("  Reason: %s\n", reason);
	handle_user_or_panic(spsr, "PABT");
}

/**
 * dabt_handler - handle data abort
 * @spsr: saved program status register at fault
 *
 * Reads DFAR (Data Fault Address) and DFSR (Data Fault Status)
 * from CP15 to determine the fault address and reason.
 */
void dabt_handler(unsigned int spsr, unsigned int pc, unsigned int *regs)
{
	unsigned int dfar, dfsr;
	__asm__ __volatile__(
		"mrc p15, 0, %0, c6, c0, 0\n"	/* DFAR */
		"mrc p15, 0, %1, c5, c0, 0\n"	/* DFSR */
		: "=r"(dfar), "=r"(dfsr) : : "memory");

	printk("\nException: Data Abort!\n");
	printk("  DFAR=0x%08x, DFSR=0x%08x\n", dfar, dfsr);
	printk("  PC=0x%08x  SPSR=0x%08x\n", pc, spsr);
	/* User register frame at the fault (regs[n] = rN). For the LVGL blend
	 * runaway hunt: r8 = row counter, r7/r9 = dest, r4/r6/r11 = mask. */
	printk("  r0=%08x r1=%08x r2=%08x r3=%08x\n",
	       regs[0], regs[1], regs[2], regs[3]);
	printk("  r4=%08x r5=%08x r6=%08x r7=%08x\n",
	       regs[4], regs[5], regs[6], regs[7]);
	printk("  r8=%08x r9=%08x r10=%08x r11=%08x r12=%08x\n",
	       regs[8], regs[9], regs[10], regs[11], regs[12]);

	const char *reason;
	switch (dfsr & 0xF) {
	case 1:  reason = "Alignment fault"; break;
	case 4:  reason = "ICache maintenance fault"; break;
	case 7:  reason = "Translation fault (page/section)"; break;
	case 9:  reason = "Domain fault (section)"; break;
	case 10: reason = "Domain fault (page)"; break;
	case 13: reason = "Permission fault (section)"; break;
	case 15: reason = "Permission fault (page)"; break;
	default: reason = "Unknown"; break;
	}
	printk("  Reason: %s\n", reason);
	handle_user_or_panic(spsr, "DABT");
}

/**
 * irq_handler - top-level interrupt handler
 *
 * Delegates to the INTC dispatch which reads the active IRQ
 * and calls the registered handler.
 */
void irq_handler(void)
{
	intc_handle_irq();
}

/**
 * fiq_handler - handle fast interrupt request
 * @spsr: saved program status register at interrupt
 */
void fiq_handler(unsigned int spsr)
{
	(void)spsr;
	printk("\nException: FIQ!\n");
	while (1)
		;
}
