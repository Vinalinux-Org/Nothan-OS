#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>

int printk(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args);

/*
 * printk() appends to a RAM ring and returns; a kernel thread feeds the UART
 * with interrupts enabled. See the long note in kernel/printk.c for why - in
 * short, the old synchronous printk held interrupts off for milliseconds per
 * line, which nothing minded until audio and networking arrived.
 *
 * printk_flush()      push everything queued out to the UART, here and now,
 *                     in the caller's context. Costs a full transmit wait.
 * printk_panic_mode() stop deferring for good. panic() only.
 * klog_init()         start the thread; until it runs, printk is synchronous.
 */
void printk_flush(void);
void printk_panic_mode(void);
void klog_init(void);

/*
 * Log levels. printk() itself is unchanged and always prints (treat as INFO)
 * — hundreds of existing call sites keep working. The pr_*() wrappers add a
 * compile-time threshold so noisy call sites (especially in scheduler/exit
 * hot paths) can be gated out entirely: below the threshold they expand to
 * nothing, so the format+UART busy-wait cost disappears at build time.
 *
 * Flip NOTHAN_LOG_LEVEL to LOG_DEBUG to bring the gated lines back for
 * debugging — nothing is deleted from the source, only compiled out.
 */
#define LOG_ERR		0
#define LOG_WARN	1
#define LOG_INFO	2
#define LOG_DEBUG	3

#ifndef NOTHAN_LOG_LEVEL
#define NOTHAN_LOG_LEVEL	LOG_INFO
#endif

/*
 * pr_err() flushes. Everything else is allowed to sit in the ring for a while;
 * an error line is not, because it is so often the last thing printed before a
 * hang, a fault, or a reset - the cases where "it will go out shortly" turns
 * into "it never went out". The flush costs a full UART wait, which is the
 * whole objection to synchronous printing, and it is accepted HERE precisely
 * because errors are rare. If they stop being rare, the log is the least of it.
 */
#if NOTHAN_LOG_LEVEL >= LOG_ERR
#define pr_err(...)	do { printk(__VA_ARGS__); printk_flush(); } while (0)
#else
#define pr_err(...)	do {} while (0)
#endif

#if NOTHAN_LOG_LEVEL >= LOG_WARN
#define pr_warn(...)	printk(__VA_ARGS__)
#else
#define pr_warn(...)	do {} while (0)
#endif

#if NOTHAN_LOG_LEVEL >= LOG_INFO
#define pr_info(...)	printk(__VA_ARGS__)
#else
#define pr_info(...)	do {} while (0)
#endif

#if NOTHAN_LOG_LEVEL >= LOG_DEBUG
#define pr_debug(...)	printk(__VA_ARGS__)
#else
#define pr_debug(...)	do {} while (0)
#endif

#endif /* _PRINTK_H */
