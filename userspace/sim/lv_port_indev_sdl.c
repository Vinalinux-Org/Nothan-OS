/*
 * sim/lv_port_indev_sdl.c — LVGL input driver via SDL2 mouse
 *
 * Replaces gui/port/lv_port_indev.c for simulator builds.
 * Registers an LVGL pointer device backed by SDL mouse events so
 * the UI can be driven by clicking in the window, not just the timer tour.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "port/lv_port_indev.h"   /* found via -I$(ROOT)/gui */

static lv_indev_t *mouse_indev;
static int32_t    last_x, last_y;
static bool       pressed;

static void mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;
	data->point.x  = last_x;
	data->point.y  = last_y;
	data->state    = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

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
	}
}

void lv_port_indev_init(void)
{
	mouse_indev = lv_indev_create();
	lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(mouse_indev, mouse_read_cb);
}
