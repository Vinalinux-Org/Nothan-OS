#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "../../lib/syscall.h"
#include "../../lib/fb.h"

/*
 * Display rotation is handled the standard LVGL way, not by a hand-rolled
 * transpose in the kernel: the display is created at the panel's NATIVE
 * landscape resolution and told it is rotated 90°, so widgets render in a
 * logical 480×800 portrait space. In flush_cb we rotate the rendered region
 * into a landscape scratch buffer with LVGL's own lv_draw_sw_rotate(), then
 * hand that already-correct buffer to the kernel, which just copies it
 * straight into the framebuffer (no rotation there).
 */
#define PHYS_W   800		/* native landscape framebuffer */
#define PHYS_H   480
#define LOG_W    480		/* logical portrait (after 90° rotation) */
#define LOG_H    800

/* Full-logical-screen render buffer: every object renders in one piece. */
static uint8_t draw_buf_1[LOG_W * LOG_H * 2];
/* Landscape scratch buffer holding the rotated output sent to the panel. */
static uint8_t rotated_buf[PHYS_W * PHYS_H * 2];

static int fb_fd = -1;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	static int first_flush = 1;
	static int first_flip  = 1;

	lv_display_rotation_t rot = lv_display_get_rotation(disp);
	lv_color_format_t     cf  = lv_display_get_color_format(disp);

	/* Rotate the rendered (portrait) region into the landscape scratch buf. */
	int32_t  src_w      = lv_area_get_width(area);
	int32_t  src_h      = lv_area_get_height(area);
	uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

	lv_area_t rarea = *area;
	lv_display_rotate_area(disp, &rarea);	/* logical → physical coords */
	uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rarea), cf);

	/* DEBUG: log each flush so we can see how the screen is tiled. */
	{
		static int fl;
		if (fl < 12) {
			char m[128];
			lv_snprintf(m, sizeof(m),
				"[FLUSH] #%d src[%d,%d,%d,%d] %dx%d -> rot[%d,%d,%d,%d] sstr=%u dstr=%u last=%d\n",
				fl, (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2,
				(int)src_w, (int)src_h,
				(int)rarea.x1, (int)rarea.y1, (int)rarea.x2, (int)rarea.y2,
				(unsigned)src_stride, (unsigned)dest_stride,
				(int)lv_display_flush_is_last(disp));
			write(m);
			fl++;
		}
	}

	lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rot, cf);

	if (fb_fd >= 0) {
		if (first_flush) { write("[GUI] first flush\n"); first_flush = 0; }

		struct fb_flush f = {
			.x1   = rarea.x1,
			.y1   = rarea.y1,
			.x2   = rarea.x2,
			.y2   = rarea.y2,
			.data = (unsigned long)rotated_buf,
			.len  = (unsigned int)(lv_area_get_width(&rarea) *
					       lv_area_get_height(&rarea) * 2),
		};
		ioctl(fb_fd, FB_FLUSH, (unsigned long)&f);

		if (lv_display_flush_is_last(disp)) {
			if (first_flip) { write("[GUI] first flip\n"); first_flip = 0; }
			ioctl(fb_fd, FB_FLIP, 0);
		}
	}
	lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
	fb_fd = open("/dev/fb0", 0);

	/* Create at the native landscape size, then rotate 90° to portrait. */
	lv_display_t *disp = lv_display_create(PHYS_W, PHYS_H);
	lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
	lv_display_set_flush_cb(disp, flush_cb);
	lv_display_set_buffers(disp, draw_buf_1, NULL,
			       sizeof(draw_buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}
