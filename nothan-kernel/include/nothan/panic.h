#ifndef _NOTHAN_PANIC_H
#define _NOTHAN_PANIC_H

/*
 * Loud, at-the-source failure for the kernel.
 *
 * panic() prints a message + a stack backtrace of the current context, then
 * halts. BUG_ON() is the kernel's assert(): if the condition holds, report
 * file:line and panic — so a broken invariant fails right where it happens,
 * instead of corrupting state and crashing somewhere unrelated later.
 */

void panic(const char *fmt, ...)
	__attribute__((noreturn, format(printf, 1, 2)));
void __bug(const char *file, int line) __attribute__((noreturn));

#define BUG()          __bug(__FILE__, __LINE__)
#define BUG_ON(cond)   do { if (cond) __bug(__FILE__, __LINE__); } while (0)

/* Kernel .text bounds (kernel.ld) + the stack-scan backtrace (traps.c),
 * shared so panic() can walk the current stack. */
extern char _stext[], _etext[];
void dump_backtrace(unsigned long *sp, unsigned long lo, unsigned long hi);

#endif /* _NOTHAN_PANIC_H */
