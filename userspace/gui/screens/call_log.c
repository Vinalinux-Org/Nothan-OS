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
#include "../widgets/swipe_row.h"
#include "../services/telephony.h"
#include "../services/modem_client.h"
#include "../../lib/string.h"

#define ROW_H      84
#define AVATAR_SZ  60

static lv_obj_t *list_obj;

/* Numbers captured at populate() time — on_row uses these directly so it
 * is immune to log_n changing (e.g. from a concurrent calllog_add) between
 * populate() and the tap. */
static char row_num_buf[32][24];

static void on_keypad(lv_event_t *e)
{
	(void)e;
	gui_log("event: recents -> keypad\n");
	nav_push(dialer_create, NULL);
}

/* Tap on a row (not a swipe) → call that number back. `user` is the row's
 * stable number buffer, so it is immune to the log shifting under us. */
static void on_dial(void *user)
{
	const char *number = (const char *)user;
	if (!number || !number[0]) return;
	if (!modem_net_registered()) {
		gui_toast("No network");
		return;
	}
	gui_logf("event: call back %s\n", number);
	telephony_dial(number);
}

static void populate(void);

/* Deferred delete — run outside event dispatch (lv_async_call) so the row
 * objects, including the delete button being clicked, are freed only after
 * LVGL finishes sending the event. */
static void delete_log_async(void *p)
{
	int idx = (int)(long)p;
	telephony_calllog_delete(idx);
	populate();
}

static void on_delete(lv_event_t *e)
{
	int idx = (int)(long)lv_event_get_user_data(e);
	lv_async_call(delete_log_async, (void *)(long)idx);
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

	/* Snapshot number into a stable buffer — on_dial reads this instead of
	 * telephony_log_get(idx) so index-shift after a concurrent calllog_add
	 * cannot cause a wrong number to be dialled. */
	strncpy(row_num_buf[idx], en->number, sizeof(row_num_buf[0]) - 1);
	row_num_buf[idx][sizeof(row_num_buf[0]) - 1] = '\0';

	lv_obj_t *del;
	lv_obj_t *row = swipe_row_create(list, ROW_H, row_num_buf[idx], &del);
	lv_obj_add_event_cb(del, on_delete, LV_EVENT_CLICKED, (void *)(long)idx);

	lv_obj_set_style_pad_hor(row, 8, 0);
	lv_obj_set_style_pad_column(row, 12, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);

	/* No address book in the demo → no initial; show a neutral phone glyph
	 * (or the name's letter once contacts exist again). */
	if (en->name[0])
		avatar_create(row, en->name[0], AVATAR_SZ, &lv_font_montserrat_28);
	else
		avatar_create_icon(row, LV_SYMBOL_CALL, AVATAR_SZ,
				   &lv_font_montserrat_28);

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
	/* Rows are about to be freed — drop stale swipe state and (re)set the tap
	 * handler for the rows built below. */
	swipe_row_reset(on_dial);
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

static void on_log_changed(void)
{
	populate();
}

static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	populate();
}

static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	list_obj = NULL;
	telephony_set_log_observer(NULL);
	swipe_row_reset(NULL);   /* drop references to rows being freed */
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

	lv_obj_add_event_cb(list_obj, swipe_row_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, NULL);

	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	/* Null list_obj only on real delete, not every navigate-away, so the
	 * retained screen refreshes on pop-back (SCREEN_LOADED) instead of
	 * no-op'ing on a nulled pointer. */
	lv_obj_add_event_cb(screen, on_screen_unloaded, LV_EVENT_DELETE, NULL);
	telephony_set_log_observer(on_log_changed);
	populate();
}
