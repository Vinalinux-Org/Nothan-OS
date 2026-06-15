#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "../lib/syscall.h"

static void build_ui(void)
{
	lv_obj_t *scr = lv_screen_active();

	lv_obj_t *label = lv_label_create(scr);
	lv_label_set_text(label, "NothanOS");
	lv_obj_center(label);
}

void main(void)
{
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	build_ui();

	unsigned long last_tick = getticks();

	while (1) {
		unsigned long now = getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		lv_task_handler();
		yield();
	}
}
