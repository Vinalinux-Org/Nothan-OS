/*
 * core/log.c - GUI event logging over UART
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include "lvgl/lvgl.h"
#include "log.h"
#include "../../lib/syscall.h"

#define GUI_LOG_PREFIX  "[GUI] "

void gui_log(const char *msg)
{
	char buf[160];
	lv_snprintf(buf, sizeof(buf), "%s%s", GUI_LOG_PREFIX, msg);
	write(buf);
}

void gui_logf(const char *fmt, ...)
{
	char buf[160];
	int n = lv_snprintf(buf, sizeof(buf), "%s", GUI_LOG_PREFIX);
	if (n < 0 || n >= (int)sizeof(buf)) {
		write(buf);
		return;
	}

	va_list ap;
	va_start(ap, fmt);
	lv_vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
	va_end(ap);
	write(buf);
}
