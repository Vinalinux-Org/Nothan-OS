/*
 * sim/lv_port_disp_sdl.c — LVGL display driver via SDL2
 *
 * Replaces gui/port/lv_port_disp.c for simulator builds.
 *
 * Buffer mode intentionally mirrors the hardware build (full-screen
 * PARTIAL buffer) so rendering behaviour is identical — the simulator
 * is the place to catch LVGL SW renderer bugs, not to hide them.
 *
 * To reproduce the original strip-clip crash, set BUG_REPRO to 1:
 * that shrinks the buffer to 10 lines and triggers the same clip-path
 * over-run seen on hardware.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"   /* found via -I$(ROOT)/gui */

#define DISP_W      480
#define DISP_H      800
#define BUG_REPRO   0   /* 1 = small buffer → reproduces the strip-clip crash */

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;

#if BUG_REPRO
/* 10-line strip — same as the original hardware crash scenario */
static uint8_t draw_buf[DISP_W * 10 * 2];
#else
/* Full-screen buffer — identical to the fixed hardware build */
static uint8_t draw_buf[DISP_W * DISP_H * 2];
#endif

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	int w = lv_area_get_width(area);
	int h = lv_area_get_height(area);

	SDL_Rect rect = { area->x1, area->y1, w, h };
	SDL_UpdateTexture(texture, &rect, px_map, w * 2);

	/* Only present after the last partial flush for this frame */
	if (lv_display_flush_is_last(disp)) {
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
	SDL_InitSubSystem(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"NothanOS MiNuong",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		DISP_W, DISP_H,
		SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	/* RGB565 matches LV_COLOR_DEPTH 16 — same byte order on LE hosts */
	texture = SDL_CreateTexture(renderer,
				    SDL_PIXELFORMAT_RGB565,
				    SDL_TEXTUREACCESS_STREAMING,
				    DISP_W, DISP_H);

	lv_display_t *d = lv_display_create(DISP_W, DISP_H);
	lv_display_set_flush_cb(d, flush_cb);
	lv_display_set_buffers(d, draw_buf, NULL,
			       sizeof(draw_buf),
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
}
