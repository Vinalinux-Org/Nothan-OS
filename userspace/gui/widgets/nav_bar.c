/*
 * widgets/nav_bar.c - Android-style bottom nav: back, home, recents
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "nav_bar.h"
#include "../theme/theme.h"

#define HOME_SIZE     14
#define RECENTS_SIZE  14

static lv_obj_t *make_button_slot(lv_obj_t *parent, lv_event_cb_t cb)
{
	/* Real lv_button so each nav key is a true clickable target. Styled
	 * transparent; a faint pressed tint gives touch feedback. The CLICKED
	 * handler is wired now — it stays inert until an input device lands,
	 * then Back/Home drive the navigation stack with no further change. */
	lv_obj_t *slot = lv_button_create(parent);
	lv_obj_remove_style_all(slot);
	lv_obj_set_size(slot, 48, NAV_BAR_HEIGHT - 8);
	lv_obj_set_style_radius(slot, RADIUS_SM, 0);
	lv_obj_set_style_bg_color(slot, theme_color(THEME_TEXT), LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(slot, LV_OPA_20, LV_STATE_PRESSED);
	lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
	if (cb)
		lv_obj_add_event_cb(slot, cb, LV_EVENT_CLICKED, NULL);
	return slot;
}

lv_obj_t *nav_bar_create(lv_obj_t *parent, lv_event_cb_t on_back,
			 lv_event_cb_t on_home, lv_event_cb_t on_recents)
{
	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), NAV_BAR_HEIGHT);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(bar, 1, 0);
	lv_obj_set_style_border_color(bar, theme_color(THEME_BORDER), 0);
	lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
	lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	/* Back: left arrow glyph */
	lv_obj_t *back_slot = make_button_slot(bar, on_back);
	lv_obj_t *back = lv_label_create(back_slot);
	lv_label_set_text(back, LV_SYMBOL_LEFT);
	lv_obj_set_style_text_color(back, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(back, &lv_font_montserrat_18, 0);
	lv_obj_center(back);

	/* Home: circle outline */
	lv_obj_t *home_slot = make_button_slot(bar, on_home);
	lv_obj_t *home = lv_obj_create(home_slot);
	lv_obj_remove_style_all(home);
	lv_obj_set_size(home, HOME_SIZE, HOME_SIZE);
	lv_obj_center(home);
	lv_obj_set_style_border_color(home, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_border_width(home, 2, 0);
	lv_obj_set_style_radius(home, LV_RADIUS_CIRCLE, 0);
	lv_obj_clear_flag(home, LV_OBJ_FLAG_SCROLLABLE);

	/* Recents: square outline */
	lv_obj_t *recents_slot = make_button_slot(bar, on_recents);
	lv_obj_t *recents = lv_obj_create(recents_slot);
	lv_obj_remove_style_all(recents);
	lv_obj_set_size(recents, RECENTS_SIZE, RECENTS_SIZE);
	lv_obj_center(recents);
	lv_obj_set_style_border_color(recents, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_border_width(recents, 2, 0);
	lv_obj_set_style_radius(recents, 2, 0);
	lv_obj_clear_flag(recents, LV_OBJ_FLAG_SCROLLABLE);

	return bar;
}
