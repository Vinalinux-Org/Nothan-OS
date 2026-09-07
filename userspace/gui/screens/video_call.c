/*
 * screens/video_call.c - the in-call screen, before there is any video in it
 *
 * A full-screen stage with the remote picture filling it and a small self-view
 * in a corner — the shape every video call has had for fifteen years, and the
 * shape the pixels will land in when they arrive.  Building it now, empty, is
 * what makes the moment they arrive a matter of drawing into a rectangle that
 * already exists rather than of inventing a layout while debugging a DMA path.
 *
 * WHAT IS NOT HERE.  No video.  The stage draws a placeholder and says so on
 * screen, because a black rectangle means "no signal", "not implemented" and
 * "the display broke" all at once, and those are three different afternoons.
 *
 * The reason it is not here is not that the video does not work — it runs at
 * 30 fps in both directions, measured.  It is that net/video_rx.c paints
 * straight onto the panel from inside the kernel while LVGL paints onto the
 * same panel through /dev/fb0 from out here.  Two writers, one framebuffer,
 * and no agreement yet about which of them owns it during a call.  That
 * decision changes what this screen does — whether it keeps drawing and
 * composites, or steps aside entirely — so it is taken before the pixels are
 * connected, not discovered afterwards.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "video_call.h"
#include "../theme/theme.h"
#include "../core/nav.h"
#include "../core/log.h"
#include "../services/chat.h"

#define SELF_W      120
#define SELF_H      160
#define SELF_MARGIN 16
#define BTN_SZ      72

static int   call_idx;
static int   call_secs;
static lv_obj_t  *dur_label;
static lv_timer_t *dur_timer;

static void on_tick(lv_timer_t *t)
{
	(void)t;
	if (!dur_label)
		return;
	call_secs++;
	lv_label_set_text_fmt(dur_label, "%02d:%02d", call_secs / 60,
			      call_secs % 60);
}

/*
 * Stop the timer before the screen it writes into is freed.
 *
 * lv_timer_t outlives a screen; the label does not.  Leaving it running is a
 * write through a freed pointer once a second — the kind of fault that shows
 * up minutes later, somewhere else, as memory that will not make sense.
 */
static void on_screen_unloaded(lv_event_t *e)
{
	(void)e;
	if (dur_timer) {
		lv_timer_delete(dur_timer);
		dur_timer = NULL;
	}
	dur_label = NULL;
}

static void on_hangup(lv_event_t *e)
{
	(void)e;
	gui_logf("event: hang up after %d s\n", call_secs);
	nav_pop();
}

static lv_obj_t *round_btn(lv_obj_t *parent, const char *symbol,
			   uint32_t color, int w, int x)
{
	lv_obj_t *btn = lv_button_create(parent);

	lv_obj_remove_style_all(btn);
	lv_obj_set_size(btn, w, w);
	lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, x, -48);
	lv_obj_set_ext_click_area(btn, 12);
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

/*
 * The rectangle the remote picture will land in: the whole screen.
 *
 * Full-bleed rather than a framed box, because that is where video_rx already
 * puts it — centred and doubled into the panel — and a layout that disagreed
 * with the code underneath would have to be redone the day they meet.
 */
static void build_stage(lv_obj_t *screen)
{
	lv_obj_t *stage = lv_obj_create(screen);

	lv_obj_remove_style_all(stage);
	lv_obj_set_size(stage, lv_pct(100), lv_pct(100));
	lv_obj_align(stage, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_bg_color(stage, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(stage, LV_OPA_COVER, 0);
	lv_obj_clear_flag(stage, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *note = lv_label_create(stage);
	lv_label_set_text(note, LV_SYMBOL_VIDEO "  no video yet");
	lv_obj_set_style_text_color(note, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(note, &lv_font_montserrat_16, 0);
	lv_obj_center(note);
}

/* The self-view, where a corner of the local camera will go. */
static void build_self_view(lv_obj_t *screen)
{
	lv_obj_t *self = lv_obj_create(screen);

	lv_obj_remove_style_all(self);
	lv_obj_set_size(self, SELF_W, SELF_H);
	lv_obj_align(self, LV_ALIGN_TOP_RIGHT, -SELF_MARGIN, SELF_MARGIN + 40);
	lv_obj_set_style_radius(self, RADIUS_MD, 0);
	lv_obj_set_style_bg_color(self, theme_color(THEME_SURFACE), 0);
	lv_obj_set_style_bg_opa(self, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(self, 1, 0);
	lv_obj_set_style_border_color(self, theme_color(THEME_BORDER), 0);
	lv_obj_clear_flag(self, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *g = lv_label_create(self);
	lv_label_set_text(g, LV_SYMBOL_IMAGE);
	lv_obj_set_style_text_color(g, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(g, &lv_font_montserrat_24, 0);
	lv_obj_center(g);
}

void video_call_create(lv_obj_t *screen, void *arg)
{
	const struct chat_peer *p;

	call_idx  = (int)(long)arg;
	call_secs = 0;
	p = chat_peer_get(call_idx);

	build_stage(screen);
	build_self_view(screen);

	/* Who, and where — the address is on screen because with one hard-coded
	 * peer it is the fact worth checking against the log. */
	lv_obj_t *name = lv_label_create(screen);
	lv_label_set_text(name, p ? p->name : "Call");
	lv_obj_set_style_text_color(name, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(name, &lv_font_montserrat_24, 0);
	lv_obj_align(name, LV_ALIGN_TOP_LEFT, 20, 40);

	lv_obj_t *addr = lv_label_create(screen);
	lv_label_set_text(addr, p ? p->addr : "");
	lv_obj_set_style_text_color(addr, theme_color(THEME_SUBTEXT), 0);
	lv_obj_set_style_text_font(addr, &lv_font_montserrat_16, 0);
	lv_obj_align(addr, LV_ALIGN_TOP_LEFT, 20, 74);

	dur_label = lv_label_create(screen);
	lv_label_set_text(dur_label, "00:00");
	lv_obj_set_style_text_color(dur_label, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(dur_label, &lv_font_montserrat_20, 0);
	lv_obj_align(dur_label, LV_ALIGN_TOP_LEFT, 20, 102);

	/*
	 * The timer counts from the moment the screen opens, which is a lie
	 * this screen is allowed to tell only until there is a call to time.
	 * When signalling lands, the clock starts when the far end answers —
	 * the label stays, the source of the number changes.
	 */
	dur_timer = lv_timer_create(on_tick, 1000, NULL);

	lv_obj_t *hangup = round_btn(screen, LV_SYMBOL_CLOSE, THEME_DANGER,
				     BTN_SZ, 0);
	lv_obj_add_event_cb(hangup, on_hangup, LV_EVENT_CLICKED, NULL);

	lv_obj_add_event_cb(screen, on_screen_unloaded,
			    LV_EVENT_SCREEN_UNLOAD_START, NULL);
}
