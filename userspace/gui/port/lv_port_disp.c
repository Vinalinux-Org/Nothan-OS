#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "../../lib/syscall.h"
#include "../../lib/fb.h"

#define DISP_HOR_RES    360
#define DISP_VER_RES    640

/* Draw buffers: 10 lines each (partial render mode) */
static lv_color_t draw_buf_1[DISP_HOR_RES * 10];
static lv_color_t draw_buf_2[DISP_HOR_RES * 10];

static int fb_fd = -1;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	if (fb_fd >= 0) {
		struct fb_flush f = {
			.x1   = area->x1,
			.y1   = area->y1,
			.x2   = area->x2,
			.y2   = area->y2,
			.data = (unsigned long)px_map,
			.len  = (unsigned int)((area->x2 - area->x1 + 1) *
					       (area->y2 - area->y1 + 1) * 2),
		};
		ioctl(fb_fd, FB_FLUSH, (unsigned long)&f);
	}
	lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
	fb_fd = open("/dev/fb0", 0);

	lv_display_t *disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
	lv_display_set_flush_cb(disp, flush_cb);
	lv_display_set_buffers(disp, draw_buf_1, draw_buf_2,
			       sizeof(draw_buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}
