/*
 * screens/active_call.c - Phone: in-call screen
 *
 * Duration is static ("00:00") for now — a live ticking timer needs an
 * lv_timer torn down on screen delete; deferred to keep the mock simple.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "active_call.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../widgets/nav_bar.h"
#include "../widgets/avatar.h"
#include "../services/telephony.h"

#define AVATAR_SZ  88

static int muted;
static lv_obj_t *mute_glyph;

static void on_mute(lv_event_t *e)
{
	(void)e;
	muted = !muted;
	lv_label_set_text(mute_glyph, muted ? LV_SYMBOL_VOLUME_MAX
					    : LV_SYMBOL_MUTE);
	telephony_mute(muted);
}

static void on_hangup(lv_event_t *e)
{
	(void)e;
	gui_log("event: hangup\n");
	telephony_hangup();
	nav_pop();
}

void active_call_create(lv_obj_t *screen, void *arg)
{
	const struct call_info *c = arg;
	const char *name   = c && c->name ? c->name : NULL;
	const char *number = c ? c->number : "";
	gui_logf("screen: active-call (%s)\n", name ? name : number);

	lv_obj_t *av = avatar_create(screen, name ? name[0] : '#', AVATAR_SZ,
				     &lv_font_montserrat_42);
	lv_obj_align(av, LV_ALIGN_TOP_MID, 0, 72);

	lv_obj_t *title = lv_label_create(screen);
	lv_label_set_text(title, name ? name : number);
	lv_obj_set_style_text_color(title, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 72 + AVATAR_SZ + 16);

	if (name) {
		lv_obj_t *sub = lv_label_create(screen);
		lv_label_set_text(sub, number);
		lv_obj_set_style_text_color(sub, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
		lv_obj_align_to(sub, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
	}

	lv_obj_t *dur = lv_label_create(screen);
	lv_label_set_text(dur, "00:00");
	lv_obj_set_style_text_color(dur, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(dur, &lv_font_montserrat_20, 0);
	lv_obj_align(dur, LV_ALIGN_TOP_MID, 0, 72 + AVATAR_SZ + 90);

	/* Hang-up: red circle, centered above the nav bar. */
	lv_obj_t *hangup = lv_button_create(screen);
	lv_obj_remove_style_all(hangup);
	lv_obj_set_size(hangup, 72, 72);
	lv_obj_align(hangup, LV_ALIGN_BOTTOM_MID, 0, -(NAV_BAR_HEIGHT + 28));
	lv_obj_set_style_radius(hangup, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(hangup, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(hangup, theme_color(THEME_DANGER), 0);
	lv_obj_add_event_cb(hangup, on_hangup, LV_EVENT_CLICKED, NULL);

	lv_obj_t *hg = lv_label_create(hangup);
	lv_label_set_text(hg, LV_SYMBOL_CALL);
	lv_obj_set_style_text_color(hg, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(hg, &lv_font_montserrat_24, 0);
	lv_obj_center(hg);

	/* Mute: surface circle to the left of hang-up. */
	muted = 0;
	lv_obj_t *mute = lv_button_create(screen);
	lv_obj_remove_style_all(mute);
	lv_obj_set_size(mute, 56, 56);
	lv_obj_align(mute, LV_ALIGN_BOTTOM_MID, -100, -(NAV_BAR_HEIGHT + 36));
	lv_obj_set_style_radius(mute, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(mute, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(mute, theme_color(THEME_SURFACE), 0);
	lv_obj_add_event_cb(mute, on_mute, LV_EVENT_CLICKED, NULL);

	mute_glyph = lv_label_create(mute);
	lv_label_set_text(mute_glyph, LV_SYMBOL_MUTE);
	lv_obj_set_style_text_color(mute_glyph, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(mute_glyph, &lv_font_montserrat_20, 0);
	lv_obj_center(mute_glyph);
}
