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

/*
 * LVGL asserts both the draw buffer and any buffer it rotates into are aligned
 * to LV_DRAW_BUF_ALIGN (lv_display_set_buffers); a plain uint8_t[] is only
 * byte-aligned, so request it explicitly — the SW renderer/rotator do word
 * accesses that assume it.
 */
/* Full-logical-screen render buffer: every object renders in one piece. */
static uint8_t draw_buf_1[LOG_W * LOG_H * 2] __attribute__((aligned(LV_DRAW_BUF_ALIGN)));
/* Landscape scratch buffer holding the rotated output sent to the panel. */
static uint8_t rotated_buf[PHYS_W * PHYS_H * 2] __attribute__((aligned(LV_DRAW_BUF_ALIGN)));

static int fb_fd = -1;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	lv_display_rotation_t rot = lv_display_get_rotation(disp);
	lv_color_format_t     cf  = lv_display_get_color_format(disp);

	/* Rotate the rendered (portrait) region into the landscape scratch buf. */
	int32_t  src_w      = lv_area_get_width(area);
	int32_t  src_h      = lv_area_get_height(area);
	uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

	lv_area_t rarea = *area;
	lv_display_rotate_area(disp, &rarea);	/* logical → physical coords */
	uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rarea), cf);

	lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rot, cf);

	if (fb_fd >= 0) {
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

		if (lv_display_flush_is_last(disp))
			ioctl(fb_fd, FB_FLIP, 0);
	}
	lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
	fb_fd = open("/dev/fb0", 0);

	/*
	 * The kernel framebuffer is the single source of truth for panel
	 * geometry: it reports its native LANDSCAPE surface (e.g. 800x480). We
	 * create the display at that physical size and rotate 270° below so
	 * widgets lay out in portrait. The static buffers above hold exactly one
	 * physical frame, so fall back to the compiled-in size if the query
	 * fails and refuse a panel that would overrun them.
	 */
	int phys_w = PHYS_W, phys_h = PHYS_H;
	struct fb_info info;
	if (fb_fd >= 0 && ioctl(fb_fd, FB_GET_INFO, (unsigned long)&info) == 0 &&
	    info.width > 0 && info.height > 0) {
		phys_w = info.width;
		phys_h = info.height;
	} else {
		write("[GUI] FB_GET_INFO failed, using built-in 800x480\n");
	}

	if ((unsigned)(phys_w * phys_h * 2) > sizeof(rotated_buf)) {
		write("[GUI] panel exceeds buffer budget, clamping to 800x480\n");
		phys_w = PHYS_W;
		phys_h = PHYS_H;
	}

	/* Create at the native landscape size, then rotate 90° to portrait. */
	lv_display_t *disp = lv_display_create(phys_w, phys_h);
	lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
	lv_display_set_flush_cb(disp, flush_cb);
	lv_display_set_buffers(disp, draw_buf_1, NULL,
			       sizeof(draw_buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	{
		char m[96];
		lv_snprintf(m, sizeof(m),
			    "[GUI] disp phys=%dx%d logical=%dx%d\n",
			    (int)phys_w, (int)phys_h,
			    (int)lv_display_get_horizontal_resolution(disp),
			    (int)lv_display_get_vertical_resolution(disp));
		write(m);
	}
}
