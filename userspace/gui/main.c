/*
 * gui/main.c - NothanOS GUI entry point
 *
 * Boots into the MiNuong splash, holds it while the progress bar fills, then
 * swaps to the home launcher. After that the LVGL task loop keeps running so
 * animations settle and a future touch driver can drive nav_push/nav_pop from
 * the home app tiles. There is no input device on hardware yet, so nothing
 * mutates the UI on its own once home is up.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "core/nav.h"
#include "screens/boot.h"
#include "screens/home.h"
#include "core/call_ui.h"
#include "services/contacts.h"
#include "services/messages.h"
#include "services/telephony.h"
#include "services/modem_client.h"
#include "../lib/syscall.h"

#ifdef GUI_MONKEY
#define BOOT_MS   800		/* soak test: skip most of the splash, get to home */
#else
#define BOOT_MS   7000		/* splash dwell; matches the bar fill in boot.c */
#endif

/* Route LVGL's own logs (incl. assert file:line) out over UART. */
static void gui_lvgl_log_cb(lv_log_level_t level, const char *buf)
{
	(void)level;
	write(buf);
}

void main(void)
{
	lv_init();
	lv_log_register_print_cb(gui_lvgl_log_cb);
	lv_port_disp_init();
	lv_port_indev_init();

	nav_init();
	contacts_init();	/* load persisted contacts before any screen reads them */
	messages_init();	/* SMS threads */
	telephony_init();	/* call log + radio */
	call_ui_init();		/* call overlay on the top layer */
	modem_client_init();	/* IPC to phone_daemon */

#ifdef GUI_MONKEY
	/* Soak test mirrors the DEMO: the mock radio/SMS injectors stay OFF so the
	 * monkey exercises exactly the demo paths (home scroll + the three apps),
	 * all driven synchronously by input. The injectors fire from lv_timers on
	 * wall-clock time and rebuild the active screen (SCREEN_LOADED) underneath
	 * the finger — an async-event-vs-interaction race that the demo (mock off)
	 * never hits. Build with -DMONKEY_MOCK to soak that path separately. */
#ifdef MONKEY_MOCK
	telephony_set_mock(1);
	messages_set_mock(1);
#else
	telephony_set_mock(0);
	messages_set_mock(0);
#endif
	/* Start every soak from an identical, clean call log (deterministic). */
	telephony_calllog_clear();
#else
	/* No real input is needed to render, so keep the mock radio/SMS injectors
	 * off — nothing should mutate the UI on its own behind the user. */
	telephony_set_mock(0);
	messages_set_mock(0);
#endif

	/* Show the splash first; home takes over once the bar has filled. */
	nav_set_root(boot_create, NULL);

	unsigned long last_tick = getticks();
	unsigned long boot_t    = last_tick;
	int           on_home   = 0;

	while (1) {
		unsigned long now = getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		if (!on_home && (now - boot_t) >= BOOT_MS) {
			nav_set_root(home_create, NULL);
			nav_show_chrome(true);
			on_home = 1;
			call_ui_on_boot_done();
		}

		modem_pump();
		lv_task_handler();
		yield();
	}
}
