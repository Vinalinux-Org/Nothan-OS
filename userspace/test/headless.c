/*
 * test/headless.c — headless LVGL render harness for AddressSanitizer
 *
 * No SDL: a malloc'd 360x640 PARTIAL draw buffer (same as hardware) and a
 * no-op flush. Drives the same navigation the demo tour does, hammering the
 * Contacts detail path that faulted on hardware, and pumps lv_timer_handler
 * so every screen actually renders. Built with -fsanitize=address and LVGL
 * forced onto clib malloc (LV_STDLIB_CLIB) so each LVGL allocation gets a
 * redzone — ASan then pinpoints the exact buffer the SW blend overruns.
 */

#include <stdlib.h>
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "core/nav.h"
#include "core/call_ui.h"
#include "screens/home.h"
#include "screens/contacts_list.h"
#include "screens/contact_detail.h"
#include "screens/contacts_add.h"
#include "screens/sms_list.h"
#include "screens/sms_chat.h"
#include "screens/call_log.h"
#include "screens/dialer.h"
#include "services/contacts.h"
#include "services/messages.h"
#include "services/telephony.h"

#define W 360
#define H 640

static uint8_t *draw_buf;

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	(void)area;
	(void)px_map;
	lv_display_flush_ready(disp);
}

/* Provided by the real port files on device/sim; stubbed here. */
void lv_port_disp_init(void)
{
	draw_buf = malloc(W * H * 2);
	lv_display_t *d = lv_display_create(W, H);
	lv_display_set_flush_cb(d, flush_cb);
	lv_display_set_buffers(d, draw_buf, NULL, W * H * 2,
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
}
void lv_port_indev_init(void) {}
void sim_register_ta(lv_obj_t *ta) { (void)ta; }

/* Fill a chunk of stack with a large non-zero pattern (0xCD), then return so
 * the render below reuses this region. If LVGL reads an uninitialised stack
 * variable, it now picks up 0xCDCDCDCD (huge) instead of a benign host value
 * — reproducing the hardware garbage-coordinate runaway under ASan. */
static void dirty_stack(void)
{
	volatile unsigned char buf[48 * 1024];
	for (int i = 0; i < (int)sizeof(buf); i++)
		buf[i] = 0xCD;
	__asm__ __volatile__("" : : "r"(buf) : "memory");
}

static void pump(int frames)
{
	for (int i = 0; i < frames; i++) {
		dirty_stack();
		lv_tick_inc(33);
		lv_timer_handler();
	}
}

int main(void)
{
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	nav_init();
	contacts_init();
	messages_init();
	telephony_init();
	call_ui_init();
	telephony_set_mock(0);
	messages_set_mock(0);

	nav_set_root(home_create, NULL);
	nav_show_chrome(true);
	pump(4);

	for (int loop = 0; loop < 100; loop++) {
		/* Home grid scroll — renders the 24 gradient+shadow tiles (heavy
		 * layer/mask usage), like the hardware tour does. */
		home_scroll_to_end(1, 300); pump(14);
		home_scroll_to_end(0, 300); pump(14);

		nav_push(contacts_list_create, NULL); pump(3);
		for (int idx = 0; idx < 6; idx++) {
			nav_push(contact_detail_create, (void *)(long)idx); pump(3);
			nav_pop(); pump(2);
		}
		nav_push(contacts_add_create, NULL); pump(3); nav_pop(); pump(2);
		nav_to_root(); pump(2);

		nav_push(sms_list_create, NULL); pump(3);
		nav_push(sms_chat_create, (void *)(long)0); pump(3); nav_pop(); pump(2);
		nav_push(sms_chat_create, (void *)(long)1); pump(3); nav_pop(); pump(2);
		nav_to_root(); pump(2);

		nav_push(call_log_create, NULL); pump(3);
		nav_push(dialer_create, NULL); pump(3); nav_pop(); pump(2);
		nav_to_root(); pump(2);

		printf("loop %d done\n", loop);
		fflush(stdout);
	}
	printf("NO CRASH after all loops\n");
	return 0;
}
