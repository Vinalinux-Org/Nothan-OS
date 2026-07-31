#ifndef _NOTHAN_SIGNAL_H
#define _NOTHAN_SIGNAL_H
/*
 * include/nothan/signal.h - signal NUMBERS only, no delivery machinery
 *
 * NothanOS has no signal handlers and no sigaction. What it has, so far, is a
 * vocabulary: a way for the kernel to say WHICH kind of death a task died of,
 * in a number everyone already knows.
 *
 * Linux numbering is used deliberately even though nothing here is
 * Linux-compatible yet. Numbering is the one part of signals that is pure
 * convention and pure cost to change later: pick a private scheme now and
 * every log line, every test and every user-space check has to be renumbered
 * the day pending/blocked bitmasks arrive. Adopting the numbers costs nothing
 * today and closes that door.
 *
 * Only the ones the kernel can actually produce are listed. Adding a constant
 * that nothing raises would be inventing a user for it.
 */

#define SIGILL		4	/* undefined instruction */
#define SIGBUS		7	/* misaligned / unmappable access */
#define SIGKILL		9	/* killed by request (sys_kill) */
#define SIGSEGV		11	/* bad memory access */

#endif /* _NOTHAN_SIGNAL_H */
