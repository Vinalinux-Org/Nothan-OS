/*
 * screens/contact_detail.c - Contacts: one contact's detail + actions
 *
 * Call/SMS are visual for now — they land on the telephony stack and the
 * SMS app respectively once those exist (mock-data phase).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "contact_detail.h"
#include "../theme/theme.h"
#include "../core/log.h"
#include "../widgets/app_header.h"
#include "../widgets/avatar.h"
#include "../widgets/nav_bar.h"
#include "../services/contacts.h"

#define AVATAR_SZ  88

static void on_call(lv_event_t *e)
{
	const struct contact *c = lv_event_get_user_data(e);
	gui_logf("event: call %s\n", c ? c->name : "?");
}

static void on_sms(lv_event_t *e)
{
	const struct contact *c = lv_event_get_user_data(e);
	gui_logf("event: sms %s\n", c ? c->name : "?");
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
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
	lv_obj_center(lbl);
	return btn;
}

void contact_detail_create(lv_obj_t *screen, void *arg)
{
	const struct contact *c = arg;
	gui_logf("screen: contact-detail (%s)\n", c ? c->name : "?");

	app_header_create(screen, NULL, LV_SYMBOL_EDIT);

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
	lv_obj_set_style_text_font(phone, &lv_font_montserrat_16, 0);
	lv_obj_align_to(phone, name, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

	/* Two actions, centered as a row just above the nav bar. */
	lv_obj_t *actions = lv_obj_create(screen);
	lv_obj_remove_style_all(actions);
	lv_obj_set_size(actions, lv_pct(100), 48);
	lv_obj_align(actions, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 24));
	lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
			      LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(actions, 12, 0);
	lv_obj_clear_flag(actions, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *call = action_button(actions, LV_SYMBOL_CALL, "Call", true);
	lv_obj_t *sms  = action_button(actions, LV_SYMBOL_ENVELOPE, "SMS", false);
	lv_obj_add_event_cb(call, on_call, LV_EVENT_CLICKED, (void *)c);
	lv_obj_add_event_cb(sms, on_sms, LV_EVENT_CLICKED, (void *)c);
}
