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
#include "../core/keyboard.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/contacts.h"

#define SEARCH_H   40
#define ROW_H      60
#define AVATAR_SZ  40

/* The scrollable list + its search box, rebuilt whenever the contacts
 * change (search edit, or returning here after an add/edit/delete). */
static lv_obj_t *list_obj;
static lv_obj_t *search_box;

static void on_add(lv_event_t *e)
{
	(void)e;
	gui_log("event: contacts add (+)\n");
	nav_push(contacts_add_create, NULL);
}

static void on_row(lv_event_t *e)
{
	/* The store index rides along as the event user-data (stable across
	 * edits, unlike a pointer into the store). */
	int idx = (int)(long)lv_event_get_user_data(e);
	const struct contact *c = contacts_get(idx);
	gui_logf("event: open contact %s\n", c ? c->name : "?");
	nav_push(contact_detail_create, (void *)(long)idx);
}

static char lc(char c)
{
	return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

/* Case-insensitive substring test. Empty needle matches everything. */
static int contains_ci(const char *hay, const char *needle)
{
	if (!needle || !needle[0])
		return 1;
	for (; *hay; hay++) {
		const char *h = hay, *n = needle;
		while (*h && *n && lc(*h) == lc(*n)) { h++; n++; }
		if (!*n)
			return 1;
	}
	return 0;
}

/* Case-insensitive name compare for display ordering: <0, 0, >0. */
static int name_cmp(const char *a, const char *b)
{
	while (*a && *b) {
		char ca = lc(*a), cb = lc(*b);
		if (ca != cb)
			return (int)ca - (int)cb;
		a++;
		b++;
	}
	return (int)lc(*a) - (int)lc(*b);
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

static void add_row(lv_obj_t *list, const struct contact *c, int idx)
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
	lv_obj_add_event_cb(row, on_row, LV_EVENT_CLICKED, (void *)(long)idx);

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

/* (Re)fill the list with contacts whose name or phone matches @filter,
 * shown alphabetically with A/B/C group headers. The store keeps stable
 * indices (unsorted); ordering is computed here for display only. */
static void populate(const char *filter)
{
	if (!list_obj)
		return;
	lv_obj_clean(list_obj);

	/* Build the display order: store indices sorted by name (≤ store cap). */
	int order[64];
	int m = 0;
	int n = contacts_count();
	for (int i = 0; i < n && m < 64; i++)
		order[m++] = i;
	for (int i = 1; i < m; i++) {
		int key = order[i], j = i - 1;
		while (j >= 0 &&
		       name_cmp(contacts_get(order[j])->name,
				contacts_get(key)->name) > 0) {
			order[j + 1] = order[j];
			j--;
		}
		order[j + 1] = key;
	}

	char group = '\0';
	for (int k = 0; k < m; k++) {
		int idx = order[k];
		const struct contact *c = contacts_get(idx);
		if (!contains_ci(c->name, filter) && !contains_ci(c->phone, filter))
			continue;
		if (c->name[0] != group) {
			group = c->name[0];
			add_group_header(list_obj, group);
		}
		add_row(list_obj, c, idx);
	}
}

static void on_search_changed(lv_event_t *e)
{
	lv_obj_t *search = lv_event_get_target(e);
	populate(lv_textarea_get_text(search));
}

/* Fired when this screen is (re)loaded — e.g. after returning from a
 * detail where the user added/edited/deleted. Rebuild to reflect changes. */
static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	populate(search_box ? lv_textarea_get_text(search_box) : NULL);
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
	lv_obj_add_event_cb(search, on_search_changed, LV_EVENT_VALUE_CHANGED, NULL);
	gui_keyboard_attach(search, LV_KEYBOARD_MODE_TEXT_LOWER);
	search_box = search;
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
	list_obj = lv_obj_create(screen);
	lv_obj_remove_style_all(list_obj);
	lv_obj_set_size(list_obj, lv_pct(100), SCREEN_H - list_top - list_bottom);
	lv_obj_align(list_obj, LV_ALIGN_TOP_MID, 0, list_top);
	lv_obj_set_style_pad_hor(list_obj, 12, 0);
	lv_obj_set_scroll_dir(list_obj, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list_obj, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(list_obj, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	/* Refresh whenever we land back here (add/edit/delete in a child). */
	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	populate(NULL); /* no filter — show everyone */
}
