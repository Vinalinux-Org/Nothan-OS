/*
 * widgets/app_header.c - App-screen header: back, title, right action
 *
 * Shared by Contacts/SMS/Phone so every app screen has the same back
 * chevron and title placement. The back key calls nav_pop() directly;
 * the right action is handed back to the caller to wire.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "app_header.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"

static void on_back(lv_event_t *e) { (void)e; gui_log("event: header back\n"); nav_pop(); }

static lv_obj_t *icon_button(lv_obj_t *parent, const char *symbol)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, 40, 40);
	lv_obj_set_style_radius(btn, RADIUS_SM, 0);
	lv_obj_set_style_bg_color(btn, theme_color(THEME_TEXT), LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *glyph = lv_label_create(btn);
	lv_label_set_text(glyph, symbol);
	lv_obj_set_style_text_color(glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(glyph, &lv_font_montserrat_18, 0);
	lv_obj_center(glyph);
	return btn;
}

lv_obj_t *app_header_create(lv_obj_t *parent, const char *title,
			    const char *right_symbol)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), APP_HEADER_HEIGHT);
	lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_hor(bar, 8, 0);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *back = icon_button(bar, LV_SYMBOL_LEFT);
	lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, NULL);

	if (title) {
		lv_obj_t *lbl = lv_label_create(bar);
		lv_label_set_text(lbl, title);
		lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
		lv_obj_center(lbl);
	}

	lv_obj_t *action = NULL;
	if (right_symbol) {
		action = icon_button(bar, right_symbol);
		lv_obj_align(action, LV_ALIGN_RIGHT_MID, 0, 0);
	}
	return action;
}
