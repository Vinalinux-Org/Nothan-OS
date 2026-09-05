/*
 * screens/chat_list.c - Chat: who there is to talk to
 *
 * Built from the same parts as the SMS list on purpose — same header, same row
 * height, same avatar, same unread badge — because to the person holding the
 * box these are the same kind of thing, and two apps that do the same job
 * should not disagree about what a conversation looks like.  What differs is
 * underneath: these rows are addresses on a network, not numbers on a radio.
 *
 * No swipe-to-delete here, unlike sms_list.  A conversation on this screen is
 * a peer that exists, not a thread that accumulated; deleting the row would
 * mean forgetting the machine, which is a different action and does not belong
 * behind the same gesture.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "chat_list.h"
#include "chat_thread.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/chat.h"

#define ROW_H      76
#define AVATAR_SZ  52

static void on_open(lv_event_t *e)
{
	int idx = (int)(long)lv_event_get_user_data(e);

	gui_logf("event: open chat %d\n", idx);
	nav_push(chat_thread_create, (void *)(long)idx);
}

static void add_row(lv_obj_t *list, const struct chat_peer *p, int idx)
{
	const struct chat_message *last = chat_peer_last(idx);
	int unread = p->unread > 0;

	lv_obj_t *row = lv_obj_create(list);
	lv_obj_remove_style_all(row);
	lv_obj_set_size(row, lv_pct(100), ROW_H);
	lv_obj_set_style_pad_hor(row, 8, 0);
	lv_obj_set_style_pad_column(row, 12, 0);
	lv_obj_set_style_radius(row, RADIUS_MD, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(row, on_open, LV_EVENT_CLICKED, (void *)(long)idx);

	avatar_create_icon(row, LV_SYMBOL_WIFI, AVATAR_SZ, &lv_font_montserrat_28);

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

	lv_obj_t *name = lv_label_create(col);
	lv_label_set_text(name, p->name);
	lv_obj_set_style_text_color(name, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);

	/*
	 * The preview falls back to the address rather than to empty space.
	 * A conversation with nothing in it still has one fact worth showing,
	 * and while there is exactly one peer and it is hard-coded, that fact
	 * is the one worth checking against the log.
	 */
	lv_obj_t *preview = lv_label_create(col);
	lv_label_set_long_mode(preview, LV_LABEL_LONG_DOT);
	lv_obj_set_width(preview, lv_pct(100));
	lv_label_set_text(preview, last ? last->text : p->addr);
	lv_obj_set_style_text_color(preview,
				    theme_color(unread ? THEME_TEXT : THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(preview, &lv_font_montserrat_16, 0);

	if (unread) {
		lv_obj_t *badge = lv_obj_create(row);
		lv_obj_remove_style_all(badge);
		lv_obj_set_size(badge, 22, 22);
		lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(badge, theme_color(THEME_ACCENT), 0);
		lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

		lv_obj_t *n = lv_label_create(badge);
		lv_label_set_text_fmt(n, "%d", p->unread);
		lv_obj_set_style_text_color(n, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(n, &lv_font_montserrat_16, 0);
		lv_obj_center(n);
	}
}

void chat_list_create(lv_obj_t *screen, void *arg)
{
	(void)arg;

	app_header_create(screen, "Chat", NULL);

	lv_obj_t *list = lv_obj_create(screen);
	lv_obj_remove_style_all(list);
	lv_obj_set_size(list, lv_pct(100),
			SCREEN_H - APP_HEADER_HEIGHT - NAV_BAR_HEIGHT);
	lv_obj_align(list, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT);
	lv_obj_set_style_pad_hor(list, 12, 0);
	lv_obj_set_style_pad_ver(list, 8, 0);
	lv_obj_set_style_pad_row(list, 8, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	for (int i = 0; i < chat_peer_count(); i++)
		add_row(list, chat_peer_get(i), i);
}
