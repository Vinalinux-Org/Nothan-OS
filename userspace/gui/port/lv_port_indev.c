#include "lvgl/lvgl.h"
#include "lv_port_indev.h"
#include "../../lib/syscall.h"

static int input_fd = -1;

/* Map ASCII → LVGL key constants */
static uint32_t map_key(char c)
{
	switch (c) {
	case '\r': case '\n': return LV_KEY_ENTER;
	case 127:  case '\b': return LV_KEY_BACKSPACE;
	case '\t':            return LV_KEY_NEXT;
	case 27:              return LV_KEY_ESC;
	/* Arrow keys: ESC [ A/B/C/D — handled as single chars here,
	 * full escape sequence parsing is TODO */
	default:
		return (uint32_t)(unsigned char)c;
	}
}

static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;

	char c;
	if (input_fd >= 0 && read(input_fd, &c, 1) == 1) {
		data->key   = map_key(c);
		data->state = LV_INDEV_STATE_PRESSED;
	} else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}

void lv_port_indev_init(void)
{
	input_fd = open("/dev/input0", 0);

	lv_indev_t *indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(indev, keypad_read_cb);
}
