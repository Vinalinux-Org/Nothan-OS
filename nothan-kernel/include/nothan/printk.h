#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>

int printk(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args);

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

#if NOTHAN_LOG_LEVEL >= LOG_ERR
#define pr_err(...)	printk(__VA_ARGS__)
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
