/*
 * screens/chat_shell.c - the Chat app's own frame
 *
 * A messaging app is not one screen, it is a place you stay: conversations,
 * the people you can reach, and who you are on the network.  This is the frame
 * that holds those three, with a bar along the bottom to move between them.
 *
 * WHY THE TABS LIVE HERE AND NOT IN nav.c.  core/nav.h is a stack — one
 * full-screen screen at a time — and it is what Phone, Messages and Contacts
 * are built on.  Turning it into a set of parallel stacks so that each tab
 * remembers its own depth is the general answer, and it would change the
 * navigation under three apps that already work in order to give a fourth
 * something it can have locally.  So the tabs swap a container inside one
 * screen, and anything opened from a tab — a thread, a call — is still pushed
 * on the stack above the whole shell, which is where a full-screen thing
 * belongs.
 *
 * The cost is that switching tabs forgets where you were inside one.  That is
 * a real difference from the app in the photograph, and the moment it starts
 * to matter is the moment nav.c should grow the parallel stacks rather than
 * this file growing a copy of them.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "chat_shell.h"
#include "chat_list.h"
#include "peer_add.h"
#include "chat_thread.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/chat.h"

#define TAB_BAR_H   64
#define ROW_H       76
#define AVATAR_SZ   52

enum { TAB_MESSAGES, TAB_CONTACTS, TAB_ME, TAB_COUNT };

static const struct {
	const char *symbol;
	const char *label;
} tabs[TAB_COUNT] = {
	{ LV_SYMBOL_ENVELOPE, "Messages" },
	{ LV_SYMBOL_LIST,     "Contacts" },
	{ LV_SYMBOL_SETTINGS, "Me"       },
};

static int        cur_tab;
static lv_obj_t  *content;
static lv_obj_t  *bar_btn[TAB_COUNT];
static lv_obj_t  *header_title;

static void show_tab(int idx);

static void on_tab(lv_event_t *e)
{
	show_tab((int)(long)lv_event_get_user_data(e));
}

static void on_add_peer(lv_event_t *e)
{
	(void)e;
	nav_push(peer_add_create, NULL);
}

static void on_open_peer(lv_event_t *e)
{
	int idx = (int)(long)lv_event_get_user_data(e);

	gui_logf("event: open chat %d from contacts\n", idx);
	nav_push(chat_thread_create, (void *)(long)idx);
}

/* ── Contacts ─────────────────────────────────────────────────────────── */

static void build_contacts(lv_obj_t *parent)
{
	lv_obj_t *list = lv_obj_create(parent);

	lv_obj_remove_style_all(list);
	lv_obj_set_size(list, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_hor(list, 12, 0);
	lv_obj_set_style_pad_ver(list, 8, 0);
	lv_obj_set_style_pad_row(list, 8, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);

	for (int i = 0; i < chat_peer_count(); i++) {
		const struct chat_peer *p = chat_peer_get(i);

		lv_obj_t *row = lv_obj_create(list);
		lv_obj_remove_style_all(row);
		lv_obj_set_size(row, lv_pct(100), ROW_H);
		lv_obj_set_style_pad_hor(row, 8, 0);
		lv_obj_set_style_pad_column(row, 12, 0);
		lv_obj_set_style_radius(row, RADIUS_MD, 0);
		lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
				      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(row, on_open_peer, LV_EVENT_CLICKED,
				    (void *)(long)i);

		avatar_create(row, p->name[0], AVATAR_SZ, &lv_font_montserrat_28);

		lv_obj_t *col = lv_obj_create(row);
		lv_obj_remove_style_all(col);
		lv_obj_set_height(col, lv_pct(100));
		lv_obj_set_flex_grow(col, 1);
		lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER,
				      LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_pad_row(col, 4, 0);
		lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

		lv_obj_t *n = lv_label_create(col);
		lv_label_set_text(n, p->name);
		lv_obj_set_style_text_color(n, theme_color(THEME_TEXT), 0);
		lv_obj_set_style_text_font(n, &lv_font_montserrat_20, 0);

		lv_obj_t *a = lv_label_create(col);
		lv_label_set_text(a, p->addr);
		lv_obj_set_style_text_color(a, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(a, &lv_font_montserrat_16, 0);
	}

	/*
	 * The add button floats over the list rather than sitting at its end,
	 * so it stays reachable once there are more contacts than fit — a
	 * button that scrolls away is a button that is missing for exactly the
	 * people who have the most of what it adds.
	 */
	lv_obj_t *add = lv_button_create(parent);
	lv_obj_remove_style_all(add);
	lv_obj_set_size(add, 56, 56);
	lv_obj_align(add, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
	lv_obj_set_ext_click_area(add, 12);
	lv_obj_set_style_radius(add, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(add, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(add, theme_color(THEME_ACCENT), 0);
	lv_obj_set_style_bg_grad_color(add, theme_color(THEME_ACCENT_2), 0);
	lv_obj_set_style_bg_grad_dir(add, LV_GRAD_DIR_VER, 0);
	lv_obj_add_event_cb(add, on_add_peer, LV_EVENT_CLICKED, NULL);

	lv_obj_t *g = lv_label_create(add);
	lv_label_set_text(g, LV_SYMBOL_PLUS);
	lv_obj_set_style_text_color(g, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(g, &lv_font_montserrat_24, 0);
	lv_obj_center(g);
}

/* ── Me ───────────────────────────────────────────────────────────────── */

static void kv_row(lv_obj_t *parent, const char *key, const char *value)
{
	lv_obj_t *row = lv_obj_create(parent);

	lv_obj_remove_style_all(row);
	lv_obj_set_size(row, lv_pct(100), 64);
	lv_obj_set_style_pad_hor(row, 12, 0);
	lv_obj_set_style_radius(row, RADIUS_MD, 0);
	lv_obj_set_style_bg_color(row, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START,
			      LV_FLEX_ALIGN_START);
	lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *k = lv_label_create(row);
	lv_label_set_text(k, key);
	lv_obj_set_style_text_color(k, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(k, &lv_font_montserrat_16, 0);

	lv_obj_t *v = lv_label_create(row);
	lv_label_set_text(v, value);
	lv_obj_set_style_text_color(v, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(v, &lv_font_montserrat_20, 0);
}

/*
 * The profile tab is this box's own address and nothing else.
 *
 * Not a placeholder for a name and a photograph — the address IS the identity
 * here, because it is what the other end has to be told for any of this to
 * work.  Until there is discovery, the most useful thing this screen can do is
 * show the number a person has to read out loud.
 */
static void build_me(lv_obj_t *parent)
{
	lv_obj_t *col = lv_obj_create(parent);

	lv_obj_remove_style_all(col);
	lv_obj_set_size(col, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(col, 16, 0);
	lv_obj_set_style_pad_row(col, 12, 0);
	lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

	avatar_create_icon(col, LV_SYMBOL_HOME, 88, &lv_font_montserrat_42);

	kv_row(col, "This box", chat_self_addr());
	kv_row(col, "Chat port", "6000");
	kv_row(col, "Transport", "reliable (seq/ack/retransmit)");
}

/* ── Shell ────────────────────────────────────────────────────────────── */

static void show_tab(int idx)
{
	if (idx < 0 || idx >= TAB_COUNT || !content)
		return;

	cur_tab = idx;
	lv_label_set_text(header_title, tabs[idx].label);

	/*
	 * Clean, then build.  Every tab's content is derived from the store, so
	 * there is nothing in a tab worth preserving across a switch — and
	 * keeping three built at once would mean three sets of rows to update
	 * whenever a message arrives, of which two nobody is looking at.
	 */
	lv_obj_clean(content);

	switch (idx) {
	case TAB_MESSAGES: chat_list_build(content); break;
	case TAB_CONTACTS: build_contacts(content);  break;
	case TAB_ME:       build_me(content);        break;
	}

	for (int i = 0; i < TAB_COUNT; i++) {
		lv_obj_t *lbl = lv_obj_get_child(bar_btn[i], 0);
		uint32_t c = (i == idx) ? THEME_ACCENT_2 : THEME_SUBTEXT;

		lv_obj_set_style_text_color(lbl, theme_color(c), 0);
	}
}

static void build_tab_bar(lv_obj_t *screen)
{
	lv_obj_t *bar = lv_obj_create(screen);

	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), TAB_BAR_H);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -NAV_BAR_HEIGHT);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
	lv_obj_set_style_border_width(bar, 1, 0);
	lv_obj_set_style_border_color(bar, theme_color(THEME_BORDER), 0);
	lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY,
			      LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	for (int i = 0; i < TAB_COUNT; i++) {
		lv_obj_t *btn = lv_button_create(bar);

		lv_obj_remove_style_all(btn);
		lv_obj_set_size(btn, 72, TAB_BAR_H - 8);
		lv_obj_set_style_radius(btn, RADIUS_SM, 0);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(btn, on_tab, LV_EVENT_CLICKED,
				    (void *)(long)i);

		lv_obj_t *g = lv_label_create(btn);
		lv_label_set_text(g, tabs[i].symbol);
		lv_obj_set_style_text_font(g, &lv_font_montserrat_24, 0);
		lv_obj_center(g);

		bar_btn[i] = btn;
	}
}

static void on_screen_loaded(lv_event_t *e)
{
	(void)e;
	/* Coming back from a thread or a call: the store may have moved on. */
	show_tab(cur_tab);
}

static void on_deleted(lv_event_t *e)
{
	(void)e;
	content      = NULL;
	header_title = NULL;
	for (int i = 0; i < TAB_COUNT; i++)
		bar_btn[i] = NULL;
}

void chat_shell_create(lv_obj_t *screen, void *arg)
{
	(void)arg;

	/*
	 * The title is kept so the tab can rename it, which is why this does
	 * not use app_header_create(): that one owns its label.
	 */
	lv_obj_t *bar = lv_obj_create(screen);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, lv_pct(100), APP_HEADER_HEIGHT);
	lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(bar, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

	header_title = lv_label_create(bar);
	lv_obj_set_style_text_color(header_title, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(header_title, &lv_font_montserrat_20, 0);
	lv_obj_center(header_title);

	content = lv_obj_create(screen);
	lv_obj_remove_style_all(content);
	lv_obj_set_size(content, lv_pct(100),
			SCREEN_H - APP_HEADER_HEIGHT - TAB_BAR_H - NAV_BAR_HEIGHT);
	lv_obj_align(content, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT);
	lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

	build_tab_bar(screen);

	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	lv_obj_add_event_cb(screen, on_deleted, LV_EVENT_DELETE, NULL);

	show_tab(TAB_MESSAGES);
}
