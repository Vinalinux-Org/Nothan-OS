/*
 * gui/main.c - NothanOS GUI demo
 *
 * No touch input yet: boot directly into the home screen, then after a
 * short delay glow the Messages tile and slide into the Messages app.
 * Stays there. Purpose is to show the LVGL toolkit working end-to-end.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "screens/home.h"
#include "screens/messages.h"
#include "widgets/app_tile.h"
#include "../lib/syscall.h"

/* Demo state machine — drives the auto navigation without input. */
enum demo_phase {
	PHASE_HOME,
	PHASE_GLOW,
	PHASE_MESSAGES,
	PHASE_DONE,
};

static enum demo_phase phase   = PHASE_HOME;
static unsigned long   phase_t = 0;       /* tick at which phase started */
static lv_obj_t       *msg_tile = NULL;   /* "Messages" tile in home grid */

static void enter_messages(void)
{
	lv_obj_t *scr = lv_screen_active();
	write("[GUI] enter: before clean\n");
	lv_obj_clean(scr);
	write("[GUI] enter: after clean\n");

	/* Minimal: just change bg to green. If screen turns green we know
	 * clean + invalidate path works; the issue is messages_create. */
	lv_obj_set_style_bg_color(scr, lv_color_hex(0x00FF00), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_invalidate(scr);
	write("[GUI] enter: green set\n");
}

static void demo_tick(unsigned long now)
{
	switch (phase) {
	case PHASE_HOME:
		if (now - phase_t >= 3000) {
			write("[GUI] enter Messages\n");
			enter_messages();
			phase = PHASE_MESSAGES;
			phase_t = now;
		}
		break;
	case PHASE_GLOW:
		phase = PHASE_DONE;
		break;
	case PHASE_MESSAGES:
		phase = PHASE_DONE;
		break;
	case PHASE_DONE:
		break;
	}
}

void main(void)
{
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	msg_tile = home_create(lv_screen_active());
	write("[GUI] ready\n");

	unsigned long last_tick = getticks();
	phase_t = last_tick;

	while (1) {
		unsigned long now = getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		demo_tick(now);
		lv_task_handler();
		yield();
	}
}
