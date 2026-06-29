/*
 * core/nav.c - Phone-style navigation stack
 *
 * Screens are LVGL screen objects (lv_obj_create(NULL)) held in a small
 * fixed stack. All transitions are instant (no lv_screen_load_anim) —
 * LVGL 9.2.2's SW masked blend over-runs the compositor layer and corrupts
 * the heap. The system nav bar sits on the display top layer.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "nav.h"
#include "log.h"
#include "keyboard.h"
#include "../theme/theme.h"
#include "../widgets/nav_bar.h"

#define NAV_MAX     8		/* deepest screen nesting we allow */

static lv_obj_t *stack[NAV_MAX];
static int       depth;
static lv_obj_t *navbar;

static void screen_bg(lv_obj_t *scr)
{
	lv_obj_set_style_bg_color(scr, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}


static void on_back(lv_event_t *e)  { (void)e; gui_log("event: nav-bar back\n"); nav_pop(); }
static void on_home(lv_event_t *e)  { (void)e; gui_log("event: nav-bar home\n"); nav_to_root(); }

void nav_init(void)
{
	/* Top layer survives screen loads, so the bar drawn here is the one
	 * system-wide nav bar across every screen. Hidden until home shows. */
	navbar = nav_bar_create(lv_layer_top(), on_back, on_home, NULL);
	lv_obj_add_flag(navbar, LV_OBJ_FLAG_HIDDEN);
	depth = 0;

	/* On-screen keyboard — also on lv_layer_top(), rendered above navbar
	 * when visible so it can overlay any screen content. */
	gui_keyboard_init();
}

void nav_show_chrome(bool show)
{
	gui_logf("nav: chrome %s\n", show ? "show" : "hide");
	if (!navbar) {
		return;
	}
	if (show) {
		lv_obj_clear_flag(navbar, LV_OBJ_FLAG_HIDDEN);
	}
	else
		lv_obj_add_flag(navbar, LV_OBJ_FLAG_HIDDEN);
}

void nav_set_root(nav_builder_fn builder, void *arg)
{
	/* Cancel any in-flight animations (e.g. the boot progress bar) before
	 * we tear the outgoing screen down — they reference its objects. */
	lv_anim_delete_all();

	lv_obj_t *scr = lv_obj_create(NULL);
	screen_bg(scr);
	builder(scr, arg);

	lv_obj_t *old_active = depth ? stack[depth - 1] : NULL;

	/* Free stacked screens below the active one; delete the active one
	 * explicitly after switching away from it. */
	for (int i = 0; i < depth - 1; i++) {
		lv_obj_delete(stack[i]);
	}

	depth = 0;
	stack[depth++] = scr;

	gui_log("nav: set-root\n");

	/* Instant load (no transition layer). The animated screen-load path
	 * composites both screens through a heap layer, and LVGL 9.2.2's SW
	 * masked blend over-runs that layer during the slide — corrupting the
	 * heap. Instant loads avoid the compositing entirely. */
	lv_screen_load(scr);
	if (old_active) {
		lv_obj_delete(old_active);
	}
}

void nav_push(nav_builder_fn builder, void *arg)
{
	if (depth >= NAV_MAX) {
		return;
	}

	lv_obj_t *scr = lv_obj_create(NULL);
	screen_bg(scr);
	builder(scr, arg);

	stack[depth++] = scr;
	gui_logf("nav: push, depth=%d\n", depth);
	lv_screen_load(scr);
}

void nav_pop(void)
{
	if (depth <= 1) {
		return;
	}

	lv_obj_t *dying = stack[depth - 1];
	depth--;
	gui_logf("nav: pop, depth=%d\n", depth);
	lv_screen_load(stack[depth - 1]);
	lv_obj_delete(dying);
}

void nav_to_root(void)
{
	if (depth <= 1) {
		return;
	}

	gui_logf("nav: to-root, depth %d->1\n", depth);

	/* Switch to the root instantly, then free the intermediates — doing
	 * it instantly avoids deleting a screen mid-animation. */
	lv_screen_load(stack[0]);
	for (int i = 1; i < depth; i++) {
		lv_obj_delete(stack[i]);
	}
	depth = 1;
}
