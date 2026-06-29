/*
 * core/call_ui.c - System call overlay (on lv_layer_top)
 *
 * Renders the telephony state as a full-screen overlay above everything:
 *   INCOMING        -> caller + Accept/Decline
 *   DIALING/ACTIVE  -> caller + status/duration + Mute/Hang up
 *   IDLE            -> hidden
 * It is a telephony observer, so it reacts to both user actions and the
 * mock radio's auto-events (incoming call, remote answer, missed timeout)
 * no matter which app screen is showing.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "call_ui.h"
#include "log.h"
#include "../theme/theme.h"
#include "../widgets/avatar.h"
#include "../services/telephony.h"
#include "lvgl/lvgl.h"

#define AVATAR_SZ  96

static lv_obj_t  *overlay;     /* full-screen container on lv_layer_top */
static lv_obj_t  *dur_label;   /* duration text while ACTIVE (else NULL) */
static lv_timer_t *dur_timer;  /* 1s tick to refresh dur_label */
static lv_obj_t  *mute_glyph;

static void on_accept(lv_event_t *e) { (void)e; telephony_answer(); }
static void on_reject(lv_event_t *e) { (void)e; telephony_reject(); }
static void on_hangup(lv_event_t *e) { (void)e; telephony_hangup(); }

static void on_mute(lv_event_t *e)
{
	(void)e;
	int on = !telephony_muted();
	telephony_mute(on);
	if (mute_glyph)
		lv_label_set_text(mute_glyph, on ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
}

static lv_obj_t *round_btn(lv_obj_t *parent, const char *symbol, uint32_t color,
			   int w, lv_align_t align, int x, int y)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, w, w);
	lv_obj_align(btn, align, x, y);
	lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(btn, theme_color(color), 0);

	lv_obj_t *g = lv_label_create(btn);
	lv_label_set_text(g, symbol);
	lv_obj_set_style_text_color(g, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(g, &lv_font_montserrat_24, 0);
	lv_obj_center(g);
	return btn;
}

/* Caller block (avatar + name + number) shared by both layouts. */
static void caller_block(const char *status)
{
	const struct call_info *c = telephony_current();
	const char *name   = c->name;
	const char *number = c->number ? c->number : "";

	lv_obj_t *st = lv_label_create(overlay);
	lv_label_set_text(st, status);
	lv_obj_set_style_text_color(st, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(st, &lv_font_montserrat_16, 0);
	lv_obj_align(st, LV_ALIGN_TOP_MID, 0, 72);

	lv_obj_t *av = avatar_create(overlay, name ? name[0] : '#', AVATAR_SZ,
				     &lv_font_montserrat_42);
	lv_obj_align(av, LV_ALIGN_TOP_MID, 0, 112);

	lv_obj_t *title = lv_label_create(overlay);
	lv_label_set_text(title, name ? name : number);
	lv_obj_set_style_text_color(title, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 112 + AVATAR_SZ + 16);

	if (name) {
		lv_obj_t *sub = lv_label_create(overlay);
		lv_label_set_text(sub, number);
		lv_obj_set_style_text_color(sub, theme_color(THEME_SUBTEXT), 0);
		lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
		lv_obj_align_to(sub, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
	}
}

static void render_incoming(void)
{
	caller_block("Incoming call");

	lv_obj_t *reject = round_btn(overlay, LV_SYMBOL_CALL, THEME_DANGER, 68,
				     LV_ALIGN_BOTTOM_MID, -70, -80);
	lv_obj_add_event_cb(reject, on_reject, LV_EVENT_CLICKED, NULL);

	lv_obj_t *accept = round_btn(overlay, LV_SYMBOL_CALL, THEME_SUCCESS, 68,
				     LV_ALIGN_BOTTOM_MID, 70, -80);
	lv_obj_add_event_cb(accept, on_accept, LV_EVENT_CLICKED, NULL);
}

static void render_in_call(int active)
{
	caller_block(active ? "" : "Calling...");

	dur_label = lv_label_create(overlay);
	lv_label_set_text(dur_label, active ? "00:00" : "...");
	lv_obj_set_style_text_color(dur_label, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(dur_label, &lv_font_montserrat_20, 0);
	lv_obj_align(dur_label, LV_ALIGN_TOP_MID, 0, 112 + AVATAR_SZ + 92);

	/* Hang up, centered low. */
	lv_obj_t *hangup = round_btn(overlay, LV_SYMBOL_CALL, THEME_DANGER, 72,
				     LV_ALIGN_BOTTOM_MID, 0, -64);
	lv_obj_add_event_cb(hangup, on_hangup, LV_EVENT_CLICKED, NULL);

	/* Mute to the left (only meaningful once connected). */
	if (active) {
		lv_obj_t *mute = round_btn(overlay, LV_SYMBOL_MUTE, THEME_SURFACE, 56,
					   LV_ALIGN_BOTTOM_MID, -100, -72);
		mute_glyph = lv_obj_get_child(mute, 0);
		lv_label_set_text(mute_glyph, telephony_muted() ? LV_SYMBOL_VOLUME_MAX
								: LV_SYMBOL_MUTE);
		lv_obj_add_event_cb(mute, on_mute, LV_EVENT_CLICKED, NULL);
	}
}

static void show(int visible)
{
	if (visible)
		lv_obj_clear_flag(overlay, LV_OBJ_FLAG_HIDDEN);
	else
		lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
}

/* Boot gate: suppress overlay during splash screen. */
static int boot_done;
void call_ui_on_boot_done(void) { boot_done = 1; }

/* Telephony observer: re-render the overlay for the new state. */
static void on_state(enum tel_state s)
{
	lv_obj_clean(overlay);
	dur_label  = NULL;
	mute_glyph = NULL;

	switch (s) {
	case TEL_INCOMING:
		render_incoming();
		if (boot_done) show(1);
		break;
	case TEL_DIALING:
		render_in_call(0);
		if (boot_done) show(1);
		break;
	case TEL_ACTIVE:
		render_in_call(1);
		if (boot_done) show(1);
		break;
	case TEL_IDLE:
	default:
		show(0);
		/* The overlay lives on the top layer, so hiding it does not
		 * reload the app screen underneath. Nudge it with SCREEN_LOADED
		 * so a data-driven screen (e.g. Recents) picks up the call that
		 * just ended — same refresh convention nav uses on pop. */
		if (boot_done) lv_obj_send_event(lv_screen_active(), LV_EVENT_SCREEN_LOADED, NULL);
		break;
	}
}

static void on_dur_tick(lv_timer_t *t)
{
	(void)t;
	if (!dur_label || telephony_state() != TEL_ACTIVE)
		return;
	unsigned int s = telephony_duration_sec();
	lv_label_set_text_fmt(dur_label, "%02u:%02u", s / 60u, s % 60u);
}

void call_ui_init(void)
{
	overlay = lv_obj_create(lv_layer_top());
	lv_obj_remove_style_all(overlay);
	lv_obj_set_size(overlay, SCREEN_W, SCREEN_H);
	lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_style_bg_color(overlay, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
	lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
	/* Absorb taps so the app behind can't be touched during a call. */
	lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);

	telephony_set_observer(on_state);
	dur_timer = lv_timer_create(on_dur_tick, 500, NULL);

	gui_log("call_ui: overlay ready\n");
}
