/*
 * screens/call_log.c - Phone: recent calls (Recents)
 *
 * The Phone app opens here. Lists recent calls newest-first; missed calls
 * stand out in red. Tapping a row calls that number back (the call overlay
 * takes over). The keypad button in the header opens the dialer. The list
 * rebuilds on every screen load so a call placed/received from elsewhere
 * shows up when returning here.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "call_log.h"
#include "dialer.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/telephony.h"

#define ROW_H      84
#define AVATAR_SZ  60

static lv_obj_t *list_obj;

static void on_keypad(lv_event_t *e)
{
	(void)e;
	gui_log("event: recents -> keypad\n");
	nav_push(dialer_create, NULL);
}

static void on_row(lv_event_t *e)
{
	int idx = (int)(long)lv_event_get_user_data(e);
	const struct call_log_entry *en = telephony_log_get(idx);
	if (!en) {
		return;
	}
	gui_logf("event: call back %s\n", en->number);
	telephony_dial(en->number);
}

/* "Outgoing · 02:14" / "Incoming · 00:45" / "Missed" */
static void fmt_subtitle(const struct call_log_entry *en, char *out, int max)
{
	const char *kind = en->type == CALL_OUTGOING ? "Outgoing" :
			   en->type == CALL_INCOMING ? "Incoming" : "Missed";
	if (en->type == CALL_MISSED || en->dur_sec == 0) {
		int i = 0;
		while (kind[i] && i < max - 1) { out[i] = kind[i]; i++; }
		out[i] = '\0';
	} else {
		lv_snprintf(out, max, "%s - %02u:%02u", kind,
			    en->dur_sec / 60u, en->dur_sec % 60u);
	}
}

static void add_row(lv_obj_t *list, const struct call_log_entry *en, int idx)
{
	int missed = (en->type == CALL_MISSED);
	const char *title = en->name[0] ? en->name : en->number;

	lv_obj_t *row = lv_button_create(list);
	lv_obj_remove_style_all(row);
	lv_obj_set_size(row, lv_pct(100), ROW_H);
	lv_obj_set_style_radius(row, RADIUS_MD, 0);
	lv_obj_set_style_bg_color(row, theme_color(THEME_TEXT), LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(row, LV_OPA_10, LV_STATE_PRESSED);
	lv_obj_set_style_pad_hor(row, 8, 0);
	lv_obj_set_style_pad_column(row, 12, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(row, on_row, LV_EVENT_CLICKED, (void *)(long)idx);

	avatar_create(row, title[0], AVATAR_SZ, &lv_font_montserrat_28);

	lv_obj_t *col = lv_obj_create(row);
	lv_obj_remove_style_all(col);
	lv_obj_set_height(col, lv_pct(100));
	lv_obj_set_flex_grow(col, 1);
	lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(col, 4, 0);	/* gap between name and subtitle */
	lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *name = lv_label_create(col);
	lv_label_set_text(name, title);
	lv_obj_set_style_text_color(name, theme_color(missed ? THEME_DANGER
							     : THEME_TEXT), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);

	char sub[40];
	fmt_subtitle(en, sub, sizeof(sub));
	lv_obj_t *subl = lv_label_create(col);
	lv_label_set_text(subl, sub);
	lv_obj_set_style_text_color(subl, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(subl, &lv_font_montserrat_16, 0);

	/* A small call glyph on the right hints "tap to call back". */
	lv_obj_t *cg = lv_label_create(row);
	lv_label_set_text(cg, LV_SYMBOL_CALL);
	lv_obj_set_style_text_color(cg, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_text_font(cg, &lv_font_montserrat_24, 0);
}

static void populate(void)
{
	if (!list_obj) {
		return;
	}
	lv_obj_clean(list_obj);

	int n = telephony_log_count();
	if (n == 0) {
		lv_obj_t *empty = lv_label_create(list_obj);
		lv_label_set_text(empty, "No recent calls");
		lv_obj_set_style_text_color(empty, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(empty, &lv_font_montserrat_18, 0);
		lv_obj_set_style_pad_top(empty, 24, 0);
		return;
	}
	for (int i = 0; i < n; i++) {
		add_row(list_obj, telephony_log_get(i), i);
	}
}

static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	populate();
}

void call_log_create(lv_obj_t *screen, void *arg)
{
	(void)arg;
	lv_obj_t *kp = app_header_create(screen, "Recents", LV_SYMBOL_KEYBOARD);
	if (kp) {
		lv_obj_add_event_cb(kp, on_keypad, LV_EVENT_CLICKED, NULL);
	}

	int list_top    = APP_HEADER_HEIGHT + 8;
	int list_bottom = NAV_BAR_HEIGHT + 8;
	list_obj = lv_obj_create(screen);
	lv_obj_remove_style_all(list_obj);
	lv_obj_set_size(list_obj, lv_pct(100), SCREEN_H - list_top - list_bottom);
	lv_obj_align(list_obj, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(list_obj, 12, 0);
	lv_obj_set_scroll_dir(list_obj, LV_DIR_VER);
	lv_obj_clear_flag(list_obj, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
	lv_obj_set_scrollbar_mode(list_obj, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_bg_color(list_obj, theme_color(THEME_SUBTEXT), LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_opa(list_obj, LV_OPA_70, LV_PART_SCROLLBAR);
	lv_obj_set_style_width(list_obj, 4, LV_PART_SCROLLBAR);
	lv_obj_set_style_radius(list_obj, 2, LV_PART_SCROLLBAR);
	lv_obj_set_style_pad_right(list_obj, 2, LV_PART_SCROLLBAR);
	lv_obj_set_flex_flow(list_obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	populate();
}
