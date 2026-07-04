/*
 * core/log.c - GUI event logging over UART
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include "lvgl/lvgl.h"
#include "log.h"
#include "../theme/theme.h"
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

static lv_obj_t  *g_toast_obj;
static lv_timer_t *g_toast_timer;

static void toast_del_cb(lv_timer_t *t)
{
	(void)t;
	if (g_toast_obj) {
		lv_obj_delete(g_toast_obj);
		g_toast_obj = NULL;
	}
	g_toast_timer = NULL;
}

void gui_toast(const char *msg)
{
	if (g_toast_timer) {
		lv_timer_delete(g_toast_timer);
		g_toast_timer = NULL;
	}
	if (g_toast_obj) {
		lv_obj_delete(g_toast_obj);
		g_toast_obj = NULL;
	}

	lv_obj_t *cont = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(cont);
	lv_obj_set_style_bg_color(cont, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(cont, theme_color(THEME_BORDER), 0);
	lv_obj_set_style_border_width(cont, 1, 0);
	lv_obj_set_style_border_opa(cont, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(cont, 12, 0);
	lv_obj_set_style_pad_hor(cont, 20, 0);
	lv_obj_set_style_pad_ver(cont, 10, 0);
	lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -48);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *lbl = lv_label_create(cont);
	lv_label_set_text(lbl, msg);
	lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
	lv_obj_center(lbl);

	g_toast_obj   = cont;
	g_toast_timer = lv_timer_create(toast_del_cb, 2000, NULL);
	lv_timer_set_repeat_count(g_toast_timer, 1);
}
