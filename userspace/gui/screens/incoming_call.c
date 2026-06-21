/*
 * screens/incoming_call.c - Phone: full-screen incoming-call overlay
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "incoming_call.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/telephony.h"

#define AVATAR_SZ  96

static void on_reject(lv_event_t *e)
{
	(void)e;
	gui_log("event: reject call\n");
	telephony_hangup();
	nav_pop();
}

static void on_accept(lv_event_t *e)
{
	const struct call_info *c = lv_event_get_user_data(e);
	gui_log("event: accept call\n");
	telephony_answer(c);
	nav_pop();
}

static lv_obj_t *action(lv_obj_t *parent, const char *symbol,
			const char *label, uint32_t color, int x_ofs)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, 68, 68);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x_ofs, -(NAV_BAR_HEIGHT + 56));
	lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(btn, theme_color(color), 0);

	lv_obj_t *glyph = lv_label_create(btn);
	lv_label_set_text(glyph, symbol);
	lv_obj_set_style_text_color(glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(glyph, &lv_font_montserrat_24, 0);
	lv_obj_center(glyph);

	lv_obj_t *cap = lv_label_create(parent);
	lv_label_set_text(cap, label);
	lv_obj_set_style_text_color(cap, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
	lv_obj_align_to(cap, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
	return btn;
}

void incoming_call_create(lv_obj_t *screen, void *arg)
{
	const struct call_info *c = arg;
	const char *name   = c && c->name ? c->name : NULL;
	const char *number = c ? c->number : "";
	gui_logf("screen: incoming-call (%s)\n", name ? name : number);

	lv_obj_t *banner = lv_label_create(screen);
	lv_label_set_text(banner, "Incoming call");
	lv_obj_set_style_text_color(banner, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(banner, &lv_font_montserrat_16, 0);
	lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 72);

	lv_obj_t *av = avatar_create(screen, name ? name[0] : '#', AVATAR_SZ,
				     &lv_font_montserrat_42);
	lv_obj_align(av, LV_ALIGN_TOP_MID, 0, 112);

	lv_obj_t *title = lv_label_create(screen);
	lv_label_set_text(title, name ? name : number);
	lv_obj_set_style_text_color(title, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 112 + AVATAR_SZ + 16);

	if (name) {
		lv_obj_t *sub = lv_label_create(screen);
		lv_label_set_text(sub, number);
		lv_obj_set_style_text_color(sub, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
		lv_obj_align_to(sub, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
	}

	lv_obj_t *reject = action(screen, LV_SYMBOL_CALL, "Decline",
				  THEME_DANGER, -70);
	lv_obj_add_event_cb(reject, on_reject, LV_EVENT_CLICKED, NULL);

	lv_obj_t *accept = action(screen, LV_SYMBOL_CALL, "Accept",
				  THEME_SUCCESS, 70);
	lv_obj_add_event_cb(accept, on_accept, LV_EVENT_CLICKED, (void *)c);
}
