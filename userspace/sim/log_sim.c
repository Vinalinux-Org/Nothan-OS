/*
 * sim/log_sim.c — gui_log/gui_logf for simulator
 *
 * Replaces gui/core/log.c; uses printf instead of the NothanOS
 * write syscall. Interface is identical to the target build.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdio.h>
#include <stdarg.h>
#include "core/log.h"   /* found via -I$(ROOT)/gui */

void gui_log(const char *msg)
{
	printf("[GUI] %s", msg);
	fflush(stdout);
}

void gui_logf(const char *fmt, ...)
{
	printf("[GUI] ");
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	fflush(stdout);
}
