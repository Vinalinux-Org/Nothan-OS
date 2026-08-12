#ifndef _NOTHAN_PANIC_H
#define _NOTHAN_PANIC_H

/*
 * Fail at the source, not downstream.
 *
 * The whole point of design-philosophy.md §1 is that a bug has to be traceable
 * from a log, and the hardest ones to trace are those where the place that
 * crashes is not the place that went wrong.  A check that fires the moment an
 * invariant breaks turns one of those into an ordinary bug: the report names
 * the line that was wrong, not the line that later tripped over it.
 *
 * panic() never returns.  It masks interrupts, dumps who was running and what
 * the machine looked like, and halts — deliberately leaving the board in the
 * state that produced the failure rather than trying to carry on.
 */

/* Print the message plus full machine context, then halt.  Never returns. */
void panic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

/*
 * Describe an address in terms of the memory map: which region it belongs to
 * and how far into it.  Exposed because the exception handlers want it too.
 */
void panic_describe_addr(const char *label, unsigned long addr);

/* Dump the running task and the runqueue.  Safe to call from a fault. */
void panic_dump_tasks(void);

#define BUG() \
	panic("BUG at %s:%d", __FILE__, __LINE__)

/*
 * BUG_ON(cond) — assert an invariant, loudly.
 *
 * Use it where a wrong value would otherwise be carried silently into code
 * that cannot tell it is wrong.  Do not use it for conditions that can happen
 * (a full ring, a failed allocation); those are errors to handle, not broken
 * assumptions.
 */
#define BUG_ON(cond)							\
	do {								\
		if (cond)						\
			panic("BUG_ON(%s) at %s:%d",			\
			      #cond, __FILE__, __LINE__);		\
	} while (0)

#endif /* _NOTHAN_PANIC_H */
