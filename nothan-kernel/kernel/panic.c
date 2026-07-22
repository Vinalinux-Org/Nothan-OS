/*
 * kernel/panic.c - unrecoverable kernel error + assert (BUG_ON)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include <nothan/panic.h>
#include <nothan/printk.h>

/**
 * panic - report an unrecoverable kernel error and halt
 * @fmt: printf-style message
 *
 * Masks interrupts (freeze — no preemption/IRQ can scribble over the dying
 * state), prints the message and a backtrace of the current stack, then
 * halts. Never returns.
 */
void panic(const char *fmt, ...)
{
	char buf[128];
	va_list ap;
	unsigned long sp;

	__asm__ __volatile__("cpsid i" : : : "memory");

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	printk("\n[PANIC] %s\n", buf);

	__asm__ __volatile__("mov %0, sp" : "=r"(sp));
	dump_backtrace((unsigned long *)sp, (unsigned long)_stext,
		       (unsigned long)_etext);

	printk("[PANIC] halted.\n");
	for (;;)
		;
}

/**
 * __bug - back end of BUG()/BUG_ON(): panic with the failing source location
 */
void __bug(const char *file, int line)
{
	panic("BUG at %s:%d", file, line);
}
