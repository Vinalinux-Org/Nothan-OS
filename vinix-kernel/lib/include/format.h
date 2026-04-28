/* ============================================================
 * format.h — kernel-side snprintf for /proc generators.
 * Shape mirrors vinixlibc subset; no floating point.
 * ============================================================ */

#ifndef KERNEL_FORMAT_H
#define KERNEL_FORMAT_H

#include "types.h"
#include <stdarg.h>

int ksnprintf(char *buf, size_t size, const char *fmt, ...);
int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

#endif
