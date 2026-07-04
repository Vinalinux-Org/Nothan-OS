/*
 * screens/sms_list.c - SMS: scrollable conversation list
 *
 * Rows show the peer, a one-line preview of the last message, and an unread
 * badge. Tapping opens the thread. The list rebuilds on every screen load so
 * a newly received message (mock receiver) or a reply sent from the thread
 * shows up here. Search filters by peer or preview text.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "sms_list.h"
#include "sms_chat.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/messages.h"
#include "../services/contacts.h"

#define SEARCH_H   48
#define ROW_H      84
#define AVATAR_SZ  60

static lv_obj_t *list_obj;
static lv_obj_t *search_box;

static char lc(char c)
{
	return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static int contains_ci(const char *hay, const char *needle)
{
	if (!needle || !needle[0]) {
		return 1;
	}
	for (; *hay; hay++) {
		const char *h = hay, *n = needle;
		while (*h && *n && lc(*h) == lc(*n)) { h++; n++; }
		if (!*n) {
			return 1;
		}
	}
	return 0;
}

static void on_compose(lv_event_t *e)
{
	(void)e;
	gui_log("event: sms compose\n");
}

static void on_row(lv_event_t *e)
{
	int idx = (int)(long)lv_event_get_user_data(e);
	const struct sms_conversation *c = sms_conversation_get(idx);
	gui_logf("event: open chat %s\n", c ? c->peer : "?");
	nav_push(sms_chat_create, (void *)(long)idx);
}

/* Resolve display name and avatar initial for a peer number.
 * Returns contact name if found in address book, raw peer otherwise.
 * init_out receives the first char to show in the avatar. */
static const char *peer_display(const char *peer, char *init_out)
{
	const struct contact *ct = contacts_find_by_phone(peer);
	if (ct && ct->name[0]) {
		*init_out = ct->name[0];
		return ct->name;
	}
	/* Unknown number — skip leading '+' for avatar initial */
	const char *p = peer;
	while (*p == '+') p++;
	*init_out = *p ? *p : '?';
	return peer;
}

static void add_row(lv_obj_t *list, const struct sms_conversation *c, int idx)
{
	int unread = c->unread > 0;

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

	char av_init;
	const char *display_name = peer_display(c->peer, &av_init);
	avatar_create(row, av_init, AVATAR_SZ, &lv_font_montserrat_28);

	/* Text column: peer on top, preview below. */
	lv_obj_t *col = lv_obj_create(row);
	lv_obj_remove_style_all(col);
	lv_obj_set_height(col, lv_pct(100));
	lv_obj_set_flex_grow(col, 1);
	lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(col, 4, 0);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *peer = lv_label_create(col);
	lv_label_set_text(peer, display_name);
	lv_obj_set_style_text_color(peer, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(peer, &lv_font_montserrat_20, 0);

	lv_obj_t *preview = lv_label_create(col);
	lv_label_set_long_mode(preview, LV_LABEL_LONG_DOT);
	lv_label_set_text(preview, sms_preview(c));
	lv_obj_set_style_text_color(preview,
				    theme_color(unread ? THEME_TEXT : THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(preview, &lv_font_montserrat_16, 0);

	/* Unread badge: small accent dot with the count. */
	if (unread) {
		lv_obj_t *badge = lv_obj_create(row);
		lv_obj_remove_style_all(badge);
		lv_obj_set_size(badge, 22, 22);
		lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(badge, theme_color(THEME_ACCENT), 0);
		lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

		lv_obj_t *n = lv_label_create(badge);
		lv_label_set_text_fmt(n, "%d", c->unread);
		lv_obj_set_style_text_color(n, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(n, &lv_font_montserrat_16, 0);
		lv_obj_center(n);
	}
}

static void populate(const char *filter)
{
	if (!list_obj) {
		return;
	}
	lv_obj_clean(list_obj);
	for (int i = 0; i < sms_conversation_count(); i++) {
		const struct sms_conversation *c = sms_conversation_get(i);
		if (!contains_ci(c->peer, filter) &&
		    !contains_ci(sms_preview(c), filter))
			continue;
		add_row(list_obj, c, i);
	}
}

static void on_search_changed(lv_event_t *e)
{
	populate(lv_textarea_get_text(lv_event_get_target(e)));
}

static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	populate(search_box ? lv_textarea_get_text(search_box) : NULL);
}

static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	list_obj   = NULL;
	search_box = NULL;
}

static void build_search(lv_obj_t *parent)
{
	lv_obj_t *search = lv_textarea_create(parent);
	lv_textarea_set_one_line(search, true);
	lv_textarea_set_placeholder_text(search, "Search");
	lv_obj_set_size(search, lv_pct(92), SEARCH_H);
	lv_obj_align(search, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 8);
	lv_obj_set_style_bg_color(search, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(search, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(search, SEARCH_H / 2, 0);
	lv_obj_set_style_border_width(search, 0, 0);
	lv_obj_set_style_text_color(search, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(search, &lv_font_montserrat_18, 0);
	lv_obj_set_style_text_color(search, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);
	lv_obj_add_event_cb(search, on_search_changed, LV_EVENT_VALUE_CHANGED, NULL);
	gui_keyboard_attach(search, LV_KEYBOARD_MODE_TEXT_LOWER);
	search_box = search;
}

void sms_list_create(lv_obj_t *screen, void *arg)
{
	(void)arg;
	lv_obj_t *compose = app_header_create(screen, "Messages", LV_SYMBOL_EDIT);
	if (compose) {
		lv_obj_add_event_cb(compose, on_compose, LV_EVENT_CLICKED, NULL);
	}

	build_search(screen);

	int list_top    = APP_HEADER_HEIGHT + 8 + SEARCH_H + 8;
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
	lv_obj_add_event_cb(screen, on_screen_unloaded, LV_EVENT_SCREEN_UNLOAD_START, NULL);
	populate(NULL);
}
