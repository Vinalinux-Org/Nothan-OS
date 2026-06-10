/*
 * arch/arm/kernel/traps.c - ARM exception handlers (abort, undefined, FIQ)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/printk.h>

/**
 * und_handler - handle undefined instruction exception
 * @spsr: saved program status register at fault
 */
void und_handler(unsigned int spsr)
{
	(void)spsr;
	printk("\nException: Undefined Instruction!\n");
	while (1)
		;
}

/**
 * pabt_handler - handle prefetch abort
 * @spsr: saved program status register at fault
 *
 * Reads IFAR (Instruction Fault Address) and IFSR (Instruction Fault
 * Status) from CP15 to determine the fault address and reason.
 */
void pabt_handler(unsigned int spsr)
{
	(void)spsr;

	unsigned int ifar, ifsr;
	__asm__ __volatile__(
		"mrc p15, 0, %0, c6, c0, 2\n"	/* IFAR */
		"mrc p15, 0, %1, c5, c0, 1\n"	/* IFSR */
		: "=r"(ifar), "=r"(ifsr) : : "memory");

	printk("\nException: Prefetch Abort!\n");
	printk("  IFAR=0x%08x, IFSR=0x%08x\n", ifar, ifsr);
	printk("  SPSR=0x%08x\n", spsr);

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

	while (1)
		;
}

/**
 * dabt_handler - handle data abort
 * @spsr: saved program status register at fault
 *
 * Reads DFAR (Data Fault Address) and DFSR (Data Fault Status)
 * from CP15 to determine the fault address and reason.
 */
void dabt_handler(unsigned int spsr)
{
	unsigned int dfar, dfsr;
	__asm__ __volatile__(
		"mrc p15, 0, %0, c6, c0, 0\n"	/* DFAR */
		"mrc p15, 0, %1, c5, c0, 0\n"	/* DFSR */
		: "=r"(dfar), "=r"(dfsr) : : "memory");

	printk("\nException: Data Abort!\n");
	printk("  DFAR=0x%08x, DFSR=0x%08x\n", dfar, dfsr);
	printk("  SPSR=0x%08x\n", spsr);

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

	while (1)
		;
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
