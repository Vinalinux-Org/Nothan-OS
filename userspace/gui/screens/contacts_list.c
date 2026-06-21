/*
 * screens/contacts_list.c - Contacts: grouped, scrollable contact list
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "contacts_list.h"
#include "contact_detail.h"
#include "contacts_add.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/contacts.h"

#define SEARCH_H   40
#define ROW_H      60
#define AVATAR_SZ  40

static void on_add(lv_event_t *e)
{
	(void)e;
	gui_log("event: contacts add (+)\n");
	nav_push(contacts_add_create, NULL);
}

static void on_row(lv_event_t *e)
{
	/* The contact pointer rides along as the event user-data. */
	const struct contact *c = lv_event_get_user_data(e);
	gui_logf("event: open contact %s\n", c ? c->name : "?");
	nav_push(contact_detail_create, (void *)c);
}

static void add_group_header(lv_obj_t *list, char letter)
{
	char s[2] = { letter, '\0' };
	lv_obj_t *lbl = lv_label_create(list);
	lv_label_set_text(lbl, s);
	lv_obj_set_style_text_color(lbl, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
	lv_obj_set_style_pad_left(lbl, 8, 0);
	lv_obj_set_style_pad_top(lbl, 8, 0);
}

static void add_row(lv_obj_t *list, const struct contact *c)
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

	avatar_create(row, c->name[0], AVATAR_SZ, &lv_font_montserrat_18);

	/* Name over phone, stacked, taking the remaining width. */
	lv_obj_t *col = lv_obj_create(row);
	lv_obj_remove_style_all(col);
	lv_obj_set_height(col, lv_pct(100));
	lv_obj_set_flex_grow(col, 1);
	lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *name = lv_label_create(col);
	lv_label_set_text(name, c->name);
	lv_obj_set_style_text_color(name, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);

	lv_obj_t *phone = lv_label_create(col);
	lv_label_set_text(phone, c->phone);
	lv_obj_set_style_text_color(phone, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(phone, &lv_font_montserrat_12, 0);
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

void contacts_list_create(lv_obj_t *screen, void *arg)
{
	(void)arg;
	gui_log("screen: contacts-list\n");

	lv_obj_t *add = app_header_create(screen, "Contacts", LV_SYMBOL_PLUS);
	if (add)
		lv_obj_add_event_cb(add, on_add, LV_EVENT_CLICKED, NULL);

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

	char group = '\0';
	for (int i = 0; i < contacts_count(); i++) {
		const struct contact *c = contacts_get(i);
		if (c->name[0] != group) {
			group = c->name[0];
			add_group_header(list, group);
		}
		add_row(list, c);
	}
}
