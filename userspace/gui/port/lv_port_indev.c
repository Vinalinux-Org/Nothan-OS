#include "lvgl/lvgl.h"
#include "lv_port_indev.h"
#include "../core/log.h"
#include "../../lib/syscall.h"

/*
 * Touch input on hardware: /dev/input0 returns a 5-byte record
 *   { x_lo, x_hi, y_lo, y_hi, pressed }
 * in the panel's RAW landscape coordinates (raw X along the 800 axis, raw Y
 * along the 480 axis).
 *
 * IMPORTANT: the display is created at the native landscape size (800x480)
 * and set to ROTATION_270, and LVGL itself rotates pointer coordinates
 * (lv_indev: lv_display_rotate_point). So we must report points in the
 * PHYSICAL (un-rotated, 800x480) space — LVGL converts them to the logical
 * 480x800 portrait space. We only scale raw -> physical here; no manual swap.
 */
#define PHYS_W	800
#define PHYS_H	480

/* Raw touch value at physical 0 / max, per axis. Tune from corner touches. */
#define RAW_X0	10
#define RAW_X1	760
#define RAW_Y0	30
#define RAW_Y1	458

static int input_fd = -1;

static lv_coord_t scale(int v, int in0, int in1, int out_max)
{
	int o = (v - in0) * out_max / (in1 - in0);
	if (o < 0) return 0;
	if (o > out_max - 1) return out_max - 1;
	return (lv_coord_t)o;
}

static void pointer_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;
	static lv_coord_t last_x = 0, last_y = 0;
	unsigned char rec[5];

	if (input_fd >= 0 && read(input_fd, rec, 5) == 5) {
		int rx = rec[0] | (rec[1] << 8);
		int ry = rec[2] | (rec[3] << 8);

		last_x = scale(rx, RAW_X0, RAW_X1, PHYS_W);
		last_y = scale(ry, RAW_Y0, RAW_Y1, PHYS_H);
		data->state = rec[4] ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
	} else {
		data->state = LV_INDEV_STATE_RELEASED;
	}

	/* Physical coords; LVGL rotates to logical. Keep last point on release. */
	data->point.x = last_x;
	data->point.y = last_y;
}

void lv_port_indev_init(void)
{
	input_fd = open("/dev/input0", 0);

	lv_indev_t *indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, pointer_read_cb);
}

void sim_register_ta(lv_obj_t *ta) { (void)ta; }
