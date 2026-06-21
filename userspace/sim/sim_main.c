/*
 * sim/sim_main.c — NothanOS GUI simulator entry point
 *
 * Shows the boot splash for SIM_BOOT_MS (shorter than hardware), then
 * transitions to the home launcher. Navigation is fully mouse-driven.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <time.h>
#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "core/nav.h"
#include "core/log.h"
#include "screens/boot.h"
#include "screens/home.h"

#define SIM_BOOT_MS  2000   /* 2 s boot splash (hardware uses 7 s) */

void sim_indev_feed(const SDL_Event *e);

static unsigned long sim_getticks(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

int main(void)
{
	SDL_Init(SDL_INIT_VIDEO);

	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	nav_init();
	nav_set_root(boot_create, NULL);
	gui_log("ready\n");

	unsigned long last_tick   = sim_getticks();
	unsigned long start_t     = last_tick;
	unsigned long mem_t       = last_tick;
	bool          home_entered = false;

	while (1) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				return 0;
			sim_indev_feed(&e);
		}

		unsigned long now = sim_getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		if (!home_entered && now - start_t >= SIM_BOOT_MS) {
			home_entered = true;
			nav_set_root(home_create, NULL);
			nav_show_chrome(true);
			gui_log("boot done -> home\n");
		}

		if (now - mem_t >= 5000) {
			mem_t = now;
			lv_mem_monitor_t mon;
			lv_mem_monitor(&mon);
			gui_logf("mem used=%u%% free=%uB frag=%u%% peak=%uB\n",
				 (unsigned)mon.used_pct, (unsigned)mon.free_size,
				 (unsigned)mon.frag_pct, (unsigned)mon.max_used);
		}

		lv_task_handler();
		SDL_Delay(1);
	}
}
