/*
 * sim/lv_port_indev_sdl.c — LVGL input driver via SDL2 mouse + keyboard
 *
 * Replaces gui/port/lv_port_indev.c for simulator builds.
 * Registers an LVGL pointer device backed by SDL mouse events so
 * the UI can be driven by clicking in the window, not just the timer tour.
 * Keyboard text input is routed to whichever textarea is currently focused.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef SIM_HEADLESS
#include <SDL2/SDL.h>		/* SIM_HEADLESS (-m32 repro): driven by sim_indev_inject, no SDL */
#endif
#include "lvgl/lvgl.h"
#include "port/lv_port_indev.h"   /* found via -I$(ROOT)/gui */

static lv_indev_t *mouse_indev;
static int32_t    last_x, last_y;
static bool       pressed;

/* Textarea that currently has focus — set via sim_register_ta callbacks. */
static lv_obj_t  *active_ta;

static void mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;
	data->point.x  = last_x;
	data->point.y  = last_y;
	data->state    = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void ta_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t       *ta   = lv_event_get_target(e);

	if (code == LV_EVENT_FOCUSED)
		active_ta = ta;
	else if (active_ta == ta)
		active_ta = NULL;   /* DEFOCUSED or DELETE */
}

void sim_register_ta(lv_obj_t *ta)
{
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_FOCUSED,   NULL);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_DEFOCUSED, NULL);
	lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_DELETE,    NULL);
}

/*
 * Programmatic pointer injection for the headless auto-tap harness
 * (sim_main.c, SIM_AUTOTAP build). Writes the same state a real SDL mouse
 * event would, so the next mouse_read_cb hands LVGL exactly that press —
 * letting an ASan build drive a deterministic press sequence with no GUI.
 */
void sim_indev_inject(int32_t x, int32_t y, bool down)
{
	last_x  = x;
	last_y  = y;
	pressed = down;
}

#ifndef SIM_HEADLESS
/* Called from the main loop's SDL_PollEvent block */
void sim_indev_feed(const SDL_Event *e)
{
	if (e->type == SDL_MOUSEMOTION) {
		last_x = e->motion.x;
		last_y = e->motion.y;
	} else if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
		last_x  = e->button.x;
		last_y  = e->button.y;
		pressed = true;
	} else if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
		pressed = false;
	} else if (e->type == SDL_TEXTINPUT && active_ta) {
		lv_textarea_add_text(active_ta, e->text.text);
	} else if (e->type == SDL_KEYDOWN && active_ta) {
		if (e->key.keysym.sym == SDLK_BACKSPACE)
			lv_textarea_delete_char(active_ta);
	}
}
#endif /* SIM_HEADLESS */

void lv_port_indev_init(void)
{
#ifndef SIM_HEADLESS
	SDL_StartTextInput();
#endif

	mouse_indev = lv_indev_create();
	lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(mouse_indev, mouse_read_cb);
}
