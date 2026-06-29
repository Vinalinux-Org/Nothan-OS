/*
 * screens/contact_detail.c - Contacts: one contact's detail + actions
 *
 * Identified by store index (not a pointer) so edit/delete can mutate the
 * store without leaving this screen holding a dangling reference. The body
 * is rebuilt on every screen load, so returning here after an edit shows
 * the updated name/phone. Call/SMS jump to the dialer and the SMS thread;
 * Edit reopens the add form in edit mode; Delete removes and returns.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "contact_detail.h"
#include "contacts_add.h"
#include "dialer.h"
#include "sms_chat.h"
#include "../theme/theme.h"
#include "../core/log.h"
#include "../core/nav.h"
#include "../widgets/app_header.h"
#include "../widgets/avatar.h"
#include "../widgets/nav_bar.h"
#include "../services/contacts.h"
#include "../services/messages.h"

#define AVATAR_SZ  88

/* The contact this screen shows. File-scope because only one detail is
 * live at a time, and the rebuild/handlers all need it. */
static int d_idx;

static void on_call(lv_event_t *e)
{
	(void)e;
	const struct contact *c = contacts_get(d_idx);
	gui_logf("event: call %s\n", c ? c->name : "?");
	nav_push(dialer_create, c ? (void *)c->phone : NULL);
}

static void on_sms(lv_event_t *e)
{
	(void)e;
	const struct contact *c = contacts_get(d_idx);
	gui_logf("event: sms %s\n", c ? c->name : "?");
	int conv = sms_conversation_find_or_create(c ? c->phone : NULL);
	if (conv >= 0) {
		nav_push(sms_chat_create, (void *)(long)conv);
	}
}

static void on_edit(lv_event_t *e)
{
	(void)e;
	gui_logf("event: edit contact #%d\n", d_idx);
	/* Add form takes idx+1 in edit mode (0 means "add new"). */
	nav_push(contacts_add_create, (void *)(long)(d_idx + 1));
}

static void on_delete(lv_event_t *e)
{
	(void)e;
	gui_logf("event: delete contact #%d\n", d_idx);
	contacts_remove(d_idx);
	nav_pop(); /* the index is gone — return to the (refreshed) list */
}

static void big_avatar(lv_obj_t *parent, char initial)
{
	lv_obj_t *av = avatar_create(parent, initial, AVATAR_SZ,
				     &lv_font_montserrat_42);
	lv_obj_align(av, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 32);
}

static lv_obj_t *action_button(lv_obj_t *parent, const char *symbol,
			       const char *text, bool primary)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, 130, 48);
	lv_obj_set_style_radius(btn, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	if (primary) {
		lv_obj_set_style_bg_color(btn, theme_color(THEME_ACCENT), 0);
		lv_obj_set_style_bg_grad_color(btn, theme_color(THEME_ACCENT_2), 0);
		lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, 0);
	} else {
		lv_obj_set_style_bg_color(btn, theme_color(THEME_SURFACE), 0);
	}
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text_fmt(lbl, "%s  %s", symbol, text);
	lv_obj_set_style_text_color(lbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
	lv_obj_center(lbl);
	return btn;
}

/* Build (or rebuild) the whole screen body for the current d_idx. */
static void rebuild(lv_obj_t *screen)
{
	lv_obj_clean(screen);

	const struct contact *c = contacts_get(d_idx);
	gui_logf("screen: contact-detail (%s)\n", c ? c->name : "?");

	lv_obj_t *edit = app_header_create(screen, NULL, LV_SYMBOL_EDIT);
	if (edit) {
		lv_obj_add_event_cb(edit, on_edit, LV_EVENT_CLICKED, NULL);
	}

	big_avatar(screen, c ? c->name[0] : '?');

	lv_obj_t *name = lv_label_create(screen);
	lv_label_set_text(name, c ? c->name : "Unknown");
	lv_obj_set_style_text_color(name, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_24, 0);
	lv_obj_align(name, LV_ALIGN_TOP_MID, 0, APP_HEADER_HEIGHT + 32 + AVATAR_SZ + 16);

	lv_obj_t *phone = lv_label_create(screen);
	lv_label_set_text_fmt(phone, "%s  %s", LV_SYMBOL_CALL,
			      c ? c->phone : "--");
	lv_obj_set_style_text_color(phone, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(phone, &lv_font_montserrat_20, 0);
	lv_obj_align_to(phone, name, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

	/* Call / SMS row, centered above the Delete button. */
	lv_obj_t *actions = lv_obj_create(screen);
	lv_obj_remove_style_all(actions);
	lv_obj_set_size(actions, lv_pct(100), 48);
	lv_obj_align(actions, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 80));
	lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(actions, 12, 0);
	lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *call = action_button(actions, LV_SYMBOL_CALL, "Call", true);
	lv_obj_t *sms  = action_button(actions, LV_SYMBOL_ENVELOPE, "SMS", false);
	lv_obj_add_event_cb(call, on_call, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(sms, on_sms, LV_EVENT_CLICKED, NULL);

	/* Delete — destructive, set apart at the bottom in red. */
	lv_obj_t *del = lv_button_create(screen);
	lv_obj_remove_style_all(del);
	lv_obj_set_size(del, 272, 44);
	lv_obj_align(del, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 20));
	lv_obj_set_style_radius(del, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(del, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(del, theme_color(THEME_DANGER), 0);
	lv_obj_clear_flag(del, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(del, on_delete, LV_EVENT_CLICKED, NULL);

	lv_obj_t *dlbl = lv_label_create(del);
	lv_label_set_text_fmt(dlbl, "%s  Delete contact", LV_SYMBOL_TRASH);
	lv_obj_set_style_text_color(dlbl, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(dlbl, &lv_font_montserrat_20, 0);
	lv_obj_center(dlbl);
}

static void on_screen_loaded(lv_event_t *e)
{
	rebuild(lv_event_get_target(e));
}

void contact_detail_create(lv_obj_t *screen, void *arg)
{
	d_idx = (int)(long)arg;
	/* Rebuild on every (re)load so an edit done in a child screen shows. */
	lv_obj_add_event_cb(screen, on_screen_loaded, LV_EVENT_SCREEN_LOADED, NULL);
	rebuild(screen);
}
