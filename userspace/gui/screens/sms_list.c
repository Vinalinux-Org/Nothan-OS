/*
 * screens/sms_list.c - SMS: scrollable conversation list
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "sms_list.h"
#include "sms_chat.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/messages.h"

#define SEARCH_H   40
#define ROW_H      66
#define AVATAR_SZ  44

static void on_compose(lv_event_t *e)
{
	(void)e;
	gui_log("event: sms compose (new)\n");
}

static void on_row(lv_event_t *e)
{
	const struct sms_conversation *c = lv_event_get_user_data(e);
	gui_logf("event: open chat %s\n", c ? c->peer : "?");
	nav_push(sms_chat_create, (void *)c);
}

static void add_row(lv_obj_t *list, const struct sms_conversation *c)
{
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
	lv_obj_add_event_cb(row, on_row, LV_EVENT_CLICKED, (void *)c);

	avatar_create(row, c->peer[0], AVATAR_SZ, &lv_font_montserrat_18);

	/* Text column: top line peer + time, bottom line preview. */
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

	lv_obj_t *top = lv_obj_create(col);
	lv_obj_remove_style_all(top);
	lv_obj_set_width(top, lv_pct(100));
	lv_obj_set_height(top, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *peer = lv_label_create(top);
	lv_label_set_text(peer, c->peer);
	lv_obj_set_style_text_color(peer, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(peer, &lv_font_montserrat_16, 0);

	lv_obj_t *time = lv_label_create(top);
	lv_label_set_text(time, c->time);
	lv_obj_set_style_text_color(time, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(time, &lv_font_montserrat_12, 0);

	lv_obj_t *preview = lv_label_create(col);
	lv_obj_set_width(preview, lv_pct(100));
	lv_label_set_long_mode(preview, LV_LABEL_LONG_DOT);
	lv_label_set_text(preview, c->preview);
	lv_obj_set_style_text_color(preview, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(preview, &lv_font_montserrat_12, 0);
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
	lv_obj_set_style_text_font(search, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(search, theme_color(THEME_SUBTEXT),
				    LV_PART_TEXTAREA_PLACEHOLDER);
}

void sms_list_create(lv_obj_t *screen, void *arg)
{
	(void)arg;
	gui_log("screen: sms-list\n");

	lv_obj_t *compose = app_header_create(screen, "Messages", LV_SYMBOL_EDIT);
	if (compose)
		lv_obj_add_event_cb(compose, on_compose, LV_EVENT_CLICKED, NULL);

	build_search(screen);

	int list_top    = APP_HEADER_HEIGHT + 8 + SEARCH_H + 8;
	int list_bottom = NAV_BAR_HEIGHT + 8;
	lv_obj_t *list = lv_obj_create(screen);
	lv_obj_remove_style_all(list);
	lv_obj_set_size(list, lv_pct(100), 640 - list_top - list_bottom);
	lv_obj_align(list, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(list, 12, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	for (int i = 0; i < sms_conversation_count(); i++)
		add_row(list, sms_conversation_get(i));
}
