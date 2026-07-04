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
#include "../services/modem_client.h"

#define BAR_H        STATUS_BAR_HEIGHT
#define BAR_PAD      16		/* side margin so content isn't on the edge */
#define ICON_GAP     26		/* spacing between the right-cluster icons */
#define SIGNAL_W     14
#define SIGNAL_H     14

static lv_obj_t *g_sig_bars[4];
static lv_obj_t *g_no_sig_label;

void status_bar_update_signal(int registered, int rssi)
{
	(void)rssi;
	if (!g_sig_bars[0]) return;   /* widget not yet created */

	if (!registered) {
		for (int i = 0; i < 4; i++)
			lv_obj_add_flag(g_sig_bars[i], LV_OBJ_FLAG_HIDDEN);
		if (g_no_sig_label)
			lv_obj_clear_flag(g_no_sig_label, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	if (g_no_sig_label)
		lv_obj_add_flag(g_no_sig_label, LV_OBJ_FLAG_HIDDEN);
	for (int i = 0; i < 4; i++) {
		lv_obj_clear_flag(g_sig_bars[i], LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_style_bg_opa(g_sig_bars[i], LV_OPA_COVER, 0);
	}
}

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
		lv_obj_set_style_bg_opa(bar, LV_OPA_30, 0);   /* dim until registered */
		lv_obj_set_style_radius(bar, 1, 0);
		lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
		g_sig_bars[i] = bar;
	}

	/* "No signal" X — shown when not registered, hidden otherwise */
	g_no_sig_label = lv_label_create(cont);
	lv_label_set_text(g_no_sig_label, LV_SYMBOL_CLOSE);
	lv_obj_set_style_text_color(g_no_sig_label, theme_color(THEME_DANGER), 0);
	lv_obj_set_style_text_font(g_no_sig_label, &lv_font_montserrat_16, 0);
	lv_obj_align(g_no_sig_label, LV_ALIGN_CENTER, 0, 0);

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
	lv_label_set_text(clock, "10:07");
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

	/* Register for future NET_REG / SIGNAL updates, then apply current state. */
	modem_set_signal_cb(status_bar_update_signal);
	status_bar_update_signal(modem_net_registered(), 0);

	return bar;
}
