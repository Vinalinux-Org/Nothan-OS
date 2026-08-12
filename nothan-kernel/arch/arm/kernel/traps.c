/*
 * arch/arm/kernel/traps.c - ARM exception handlers (abort, undefined, FIQ)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/printk.h>
#include <nothan/sched.h>
#include <nothan/panic.h>

#define SPSR_MODE_MASK	0x1F
#define MODE_USER	0x10	/* ARM user mode */

/*
 * Decode an ARM short-descriptor fault status.
 *
 * The status is five bits, not four: FS[3:0] sit in bits 3:0 but FS[4] lives
 * up at bit 10.  The previous version masked with 0xF and dropped FS[4]
 * entirely, and its table was missing 5 — translation fault on a section,
 * which is the single most common fault a kernel produces.  Every abort in
 * this project's logs so far has been reported as "Unknown" for that reason.
 *
 * Ref: ARM ARM B3.13.3 (short-descriptor format), DFSR/IFSR.
 */
static const char *fault_reason(unsigned int fsr)
{
	unsigned int fs = (fsr & 0xF) | ((fsr >> 6) & 0x10);

	switch (fs) {
	case 0x01: return "Alignment fault";
	case 0x02: return "Debug event";
	case 0x03: return "Access flag fault (section)";
	case 0x04: return "Instruction cache maintenance fault";
	case 0x05: return "Translation fault (section)";
	case 0x06: return "Access flag fault (page)";
	case 0x07: return "Translation fault (page)";
	case 0x08: return "Synchronous external abort";
	case 0x09: return "Domain fault (section)";
	case 0x0B: return "Domain fault (page)";
	case 0x0C: return "External abort on table walk (1st level)";
	case 0x0D: return "Permission fault (section)";
	case 0x0E: return "External abort on table walk (2nd level)";
	case 0x0F: return "Permission fault (page)";
	case 0x16: return "Asynchronous external abort";
	default:   return "Unknown";
	}
}

/*
 * If the exception originated in user mode, kill the faulting task and
 * reschedule. Only panic if the fault came from kernel mode — kernel
 * exceptions are unrecoverable.
 */
static void handle_user_or_panic(unsigned int spsr, const char *tag)
{
	if ((spsr & SPSR_MODE_MASK) == MODE_USER) {
		panic_dump_tasks();
		printk("  [%s] killing user task \"%s\" pid=%d\n",
		       tag, runqueue.curr->comm, runqueue.curr->pid);
		do_exit(-1);
		/* NOTREACHED */
	}

	/* panic() dumps the task context itself — do not do it twice. */
	panic("%s in kernel mode", tag);
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

	printk("\nException: Prefetch Abort\n");
	printk("  IFSR=0x%08x  %s\n", ifsr, fault_reason(ifsr));
	printk("  LR_usr=0x%08x  SPSR=0x%08x\n", lr_usr, spsr);
	panic_describe_addr("IFAR", ifar);
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

	printk("\nException: Data Abort\n");
	printk("  DFSR=0x%08x  %s on %s\n", dfsr, fault_reason(dfsr),
	       (dfsr & (1u << 11)) ? "write" : "read");
	printk("  PC=0x%08x  SPSR=0x%08x\n", pc, spsr);
	panic_describe_addr("DFAR", dfar);
	/* User register frame at the fault (regs[n] = rN). For the LVGL blend
	 * runaway hunt: r8 = row counter, r7/r9 = dest, r4/r6/r11 = mask. */
	printk("  r0=%08x r1=%08x r2=%08x r3=%08x\n",
	       regs[0], regs[1], regs[2], regs[3]);
	printk("  r4=%08x r5=%08x r6=%08x r7=%08x\n",
	       regs[4], regs[5], regs[6], regs[7]);
	printk("  r8=%08x r9=%08x r10=%08x r11=%08x r12=%08x\n",
	       regs[8], regs[9], regs[10], regs[11], regs[12]);

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
	panic("unexpected FIQ (no FIQ source is configured)");
}
