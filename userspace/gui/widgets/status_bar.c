/*
 * widgets/status_bar.c - Top status bar (clock left, brand center,
 * signal/wifi/bat right)
 *
 * Clock is static — no RTC yet, so showing a ticking value would just
 * count from boot and be misleading.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "status_bar.h"
#include "../theme/theme.h"

#define BAR_H        STATUS_BAR_HEIGHT
#define BAR_PAD      16		/* side margin so content isn't on the edge */
#define ICON_GAP     26		/* spacing between the right-cluster icons */
#define SIGNAL_W     14
#define SIGNAL_H     14

static lv_obj_t *signal_widget_create(lv_obj_t *parent)
{
	lv_obj_t *cont = lv_obj_create(parent);
	lv_obj_remove_style_all(cont);
	lv_obj_set_size(cont, SIGNAL_W, SIGNAL_H);
	lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

	static const int heights[4] = { 5, 8, 11, 14 };
	for (int i = 0; i < 4; i++) {
		lv_obj_t *bar = lv_obj_create(cont);
		lv_obj_remove_style_all(bar);
		lv_obj_set_size(bar, 2, heights[i]);
		lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, i * 4, 0);
		lv_obj_set_style_bg_color(bar, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
		lv_obj_set_style_radius(bar, 1, 0);
		lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
	}
	return cont;
}

lv_obj_t *status_bar_create(lv_obj_t *parent)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), BAR_H);
	lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_hor(bar, BAR_PAD, 0);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *clock = lv_label_create(bar);
	lv_label_set_text(clock, "12:00");
	lv_obj_set_style_text_color(clock, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(clock, &lv_font_montserrat_16, 0);
	lv_obj_align(clock, LV_ALIGN_LEFT_MID, 0, 0);

	lv_obj_t *brand = lv_label_create(bar);
	lv_label_set_text(brand, "MyNuong");
	lv_obj_set_style_text_color(brand, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(brand, &lv_font_montserrat_16, 0);
	lv_obj_align(brand, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *batt = lv_label_create(bar);
	lv_label_set_text(batt, LV_SYMBOL_BATTERY_FULL);
	lv_obj_set_style_text_color(batt, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(batt, &lv_font_montserrat_16, 0);
	lv_obj_align(batt, LV_ALIGN_RIGHT_MID, 0, 0);

	lv_obj_t *wifi = lv_label_create(bar);
	lv_label_set_text(wifi, LV_SYMBOL_WIFI);
	lv_obj_set_style_text_color(wifi, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(wifi, &lv_font_montserrat_16, 0);
	lv_obj_align(wifi, LV_ALIGN_RIGHT_MID, -ICON_GAP, 0);

	lv_obj_t *signal = signal_widget_create(bar);
	lv_obj_align(signal, LV_ALIGN_RIGHT_MID, -2 * ICON_GAP, 0);

	return bar;
}
