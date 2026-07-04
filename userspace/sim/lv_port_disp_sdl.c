/*
 * sim/lv_port_disp_sdl.c — LVGL display driver via SDL2
 *
 * Replaces gui/port/lv_port_disp.c for simulator builds.
 *
 * Two builds share this file:
 *
 *  - Interactive (default): a plain 480×800 portrait display drawn straight
 *    into an SDL window. No rotation — the simulator is square-on so a human
 *    can click it.
 *
 *  - ASan repro (SIM_AUTOTAP, `make asan`): mirrors the HARDWARE port exactly
 *    — the display is created at the panel's native 800×480 landscape and
 *    told it is rotated 270°, so widgets render in logical 480×800 portrait
 *    and flush_cb rotates each region into a landscape scratch with
 *    lv_draw_sw_rotate(), just like gui/port/lv_port_disp.c. Both the render
 *    buffer and the rotate scratch are malloc'd at their EXACT frame size so
 *    AddressSanitizer guards the byte just past each frame — catching any
 *    over-run that the hardware's oversized static buffers silently absorb.
 *    No SDL window is opened; this path is headless and deterministic.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"   /* found via -I$(ROOT)/gui */

#ifdef SIM_AUTOTAP
/* ---- HW-faithful, rotated, over-run-instrumented path (host repro) ----
 *
 * Mirrors gui/port/lv_port_disp.c exactly: 800x480 physical, ROTATION_270,
 * flush_cb rotates each region with lv_draw_sw_rotate().
 *
 *  - default (`make asan`):  buffers are exact-size malloc()s, so ASan's
 *    redzone traps an over-run of the first byte past them.
 *  - SIM_GUARD (`make guard`): buffers are end-aligned against a PROT_NONE
 *    guard page, so even a FAR over-run (the ~3.7 KB-past write the hardware
 *    abort shows) faults exactly at the buffer end — what ASan redzones miss.
 *    The rotate scratch is a single full-screen buffer, like the hardware.
 */

#include <stdlib.h>

#ifdef SIM_GUARD
#include "nothan_guard.h"
#endif

#define PHYS_W   800		/* native landscape framebuffer */
#define PHYS_H   480
#define LOG_W    480		/* logical portrait (after 270° rotation) */
#define LOG_H    800

static uint8_t *draw_buf_1;	/* exact logical frame */
#ifdef SIM_GUARD
static uint8_t *rotated_buf;	/* one full-screen scratch, guard page after */
#endif

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	lv_display_rotation_t rot = lv_display_get_rotation(disp);
	lv_color_format_t     cf  = lv_display_get_color_format(disp);

	int32_t  src_w      = lv_area_get_width(area);
	int32_t  src_h      = lv_area_get_height(area);
	uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

	lv_area_t rarea = *area;
	lv_display_rotate_area(disp, &rarea);	/* logical → physical coords */
	uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rarea), cf);

#ifdef SIM_GUARD
	/* Rotate into the full-screen scratch at offset 0, exactly like the
	 * hardware port; the guard page sits just past the full frame. */
	lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rot, cf);
#else
	/* Exact-per-region malloc so an ASan redzone butts the region end. */
	size_t   dest_sz  = (size_t)lv_area_get_width(&rarea) *
			    lv_area_get_height(&rarea) * 2;
	uint8_t *rotated  = malloc(dest_sz);
	lv_draw_sw_rotate(px_map, rotated, src_w, src_h, src_stride, dest_stride, rot, cf);
	free(rotated);
#endif
	lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
#ifdef SIM_GUARD
	draw_buf_1  = nothan_guarded_alloc((size_t)LOG_W * LOG_H * 2);
	rotated_buf = nothan_guarded_alloc((size_t)PHYS_W * PHYS_H * 2);
#else
	/* Exact logical-frame size — no slack, so ASan guards the frame end. */
	draw_buf_1 = malloc((size_t)LOG_W * LOG_H * 2);
#endif

	lv_display_t *disp = lv_display_create(PHYS_W, PHYS_H);
	lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
	lv_display_set_flush_cb(disp, flush_cb);
	lv_display_set_buffers(disp, draw_buf_1, NULL,
			       (uint32_t)LOG_W * LOG_H * 2,
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
}

#else
/* ---- Interactive SDL window path (default) ---------------------------- */

#include <SDL2/SDL.h>

#define DISP_W      480
#define DISP_H      800

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;

/* Full-screen buffer — identical to the fixed hardware build */
static uint8_t draw_buf[DISP_W * DISP_H * 2];

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
		"NothanOS MyNuong",
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

#endif /* SIM_AUTOTAP */
