#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>

int printk(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));

int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args);

#endif /* _PRINTK_H */
