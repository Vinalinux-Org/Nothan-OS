/*
 * gui/main.c - NothanOS GUI entry point
 *
 * Boots into the splash, holds it while the progress bar fills, then
 * swaps to the home launcher. The LVGL task loop keeps running with
 * touch input and the modem backend pump.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "core/nav.h"
#include "core/log.h"
#include "screens/boot.h"
#include "screens/home.h"
#include "core/call_ui.h"
#include "services/messages.h"
#include "services/telephony.h"
#include "services/modem_client.h"
#include "../lib/syscall.h"

#define BOOT_MS   7000		/* splash dwell; matches the bar fill in boot.c */

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

	/* SMS + call log persist on the SD card and load here; there is no
	 * boot-time wipe, so a swipe-deleted thread/call stays gone across
	 * reboots. The demo has no address book — Phone/Messages show numbers. */
	messages_init();
	telephony_init();
	call_ui_init();
	modem_client_init();

	/* Mock radio/SMS injectors stay off — nothing mutates the UI
	 * on its own behind the user. The real modem backend drives
	 * telephony through modem_client.c. */
	telephony_set_mock(0);
	messages_set_mock(0);

	/* Show the splash first; home takes over once the bar has filled. */
	nav_set_root(boot_create, NULL);

	unsigned long last_tick = getticks();
	unsigned long boot_t    = last_tick;
	unsigned long mem_t     = last_tick;
	int           on_home   = 0;

	while (1) {
		unsigned long now = getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		/* Pool-usage heartbeat — surfaces LVGL heap pressure so an
		 * lv_array_at/data==NULL assert can be traced to exhaustion. */
		if (now - mem_t >= 3000) {
			mem_t = now;
			lv_mem_monitor_t mon;
			lv_mem_monitor(&mon);
			gui_logf("mem used=%u%% free=%uB frag=%u%% peak=%uB\n",
				 (unsigned)mon.used_pct, (unsigned)mon.free_size,
				 (unsigned)mon.frag_pct, (unsigned)mon.max_used);
		}

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
