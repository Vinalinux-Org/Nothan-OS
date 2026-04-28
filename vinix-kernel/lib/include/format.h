/*
 * lib/include/format.h — kernel snprintf for procfs generators
 *
 * ksnprintf/kvsnprintf: no floating point, no stream/fd sink.
 */

#ifndef KERNEL_FORMAT_H
#define KERNEL_FORMAT_H

#include "types.h"
#include <stdarg.h>

int ksnprintf(char *buf, size_t size, const char *fmt, ...);
int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

#endif
