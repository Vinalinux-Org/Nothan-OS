/*
 * widgets/status_bar.c - Top status bar (clock, title, signal, battery)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "status_bar.h"

#define BAR_H        32
#define BAR_BG_COLOR 0x1A1A1A
#define BAR_FG_COLOR 0xF0F0F0

lv_obj_t *status_bar_create(lv_obj_t *parent)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_set_size(bar, lv_pct(100), BAR_H);
	lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, lv_color_hex(BAR_BG_COLOR), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(bar, 0, 0);
	lv_obj_set_style_radius(bar, 0, 0);
	lv_obj_set_style_pad_all(bar, 6, 0);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	/* Clock — left */
	lv_obj_t *clock = lv_label_create(bar);
	lv_label_set_text(clock, "09:41");
	lv_obj_set_style_text_color(clock, lv_color_hex(BAR_FG_COLOR), 0);
	lv_obj_align(clock, LV_ALIGN_LEFT_MID, 0, 0);

	/* Title — center */
	lv_obj_t *title = lv_label_create(bar);
	lv_label_set_text(title, "NothanOS");
	lv_obj_set_style_text_color(title, lv_color_hex(BAR_FG_COLOR), 0);
	lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

	/* Signal + battery — right (text symbols from lv_symbols) */
	lv_obj_t *icons = lv_label_create(bar);
	lv_label_set_text(icons, LV_SYMBOL_WIFI "  " LV_SYMBOL_BATTERY_FULL);
	lv_obj_set_style_text_color(icons, lv_color_hex(BAR_FG_COLOR), 0);
	lv_obj_align(icons, LV_ALIGN_RIGHT_MID, 0, 0);

	return bar;
}
