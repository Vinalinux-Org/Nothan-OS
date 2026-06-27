/*
 * sim/sim_main.c — NothanOS GUI simulator entry point
 *
 * Shows the boot splash for SIM_BOOT_MS (shorter than hardware), then
 * transitions to the home launcher. Navigation is fully mouse-driven.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <time.h>
#include <stdlib.h>
#ifndef SIM_HEADLESS
#include <SDL2/SDL.h>		/* SIM_HEADLESS: -m32 ILP32 repro, no SDL/window */
#endif
#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "core/nav.h"
#include "core/log.h"
#include "screens/boot.h"
#include "screens/home.h"
#include "core/call_ui.h"
#include "services/contacts.h"
#include "services/messages.h"
#include "services/telephony.h"

#define SIM_BOOT_MS  BOOT_DURATION_MS

#ifndef SIM_HEADLESS
void sim_indev_feed(const SDL_Event *e);
#endif

static unsigned long sim_getticks(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

#ifdef SIM_AUTOTAP
/*
 * Deterministic auto-tap harness for the host repro builds (`make asan` /
 * `make builtin`).
 *
 * The hardware Data Abort fires the instant a home tile is first pressed:
 * rendering the badge's PRESSED state is reported to over-run an LVGL buffer
 * (DFAR = __bss_end). This drives the same presses/scrolls with a virtual
 * clock — no GUI, fully reproducible — so AddressSanitizer (CLIB redzones, or
 * the work_mem global redzone under SIM_BUILTIN) and the TLSF integrity check
 * can catch the over-run on the host if it is allocator-independent.
 *
 * The SIM_AUTOTAP display mirrors the panel exactly: created at 800x480 with
 * ROTATION_270, so LVGL rotates every pointer sample from PHYSICAL landscape
 * into LOGICAL portrait (lv_display_rotate_point), just like hardware. The
 * autotap reasons in logical tile coordinates, so at_press() undoes that
 * rotation and feeds physical samples the way the real touch driver does.
 */
extern void sim_indev_inject(int32_t x, int32_t y, bool down);

/* ROTATION_270, physical hor_res = 800:  logical(lx,ly) = (py, 799 - px),
 * so to land a press on logical (lx,ly) feed physical (799 - ly, lx). */
#define AT_PHYS_HOR  800
static void at_press(int lx, int ly, bool down)
{
	sim_indev_inject(AT_PHYS_HOR - 1 - ly, lx, down);
}

static void at_pump(int frames)
{
	for (int i = 0; i < frames; i++) {
		lv_tick_inc(33);	/* one full LV_DEF_REFR_PERIOD per frame */
		lv_task_handler();
	}
}

/* After each action, verify the heap is still intact. With the built-in TLSF
 * pool (SIM_BUILTIN) this walks every block and reports the FIRST action that
 * corrupts the pool — the exact reproduction of the hardware over-run. With
 * CLIB it is a no-op (returns OK), so the check is free in the ASan build. */
static void at_check(const char *where)
{
	if (lv_mem_test() != LV_RESULT_OK) {
		gui_logf("autotap: *** HEAP CORRUPT after: %s ***\n", where);
		exit(2);
	}
}

static void at_tap(int x, int y)
{
	gui_logf("autotap: tap (%d,%d)\n", x, y);
	at_press(x, y, true);
	at_pump(3);		/* press registers, then PRESSED state renders */
	at_press(x, y, false);
	at_pump(3);		/* release → maybe CLICKED → nav_push */
	nav_to_root();		/* return to home if a builder navigated away */
	at_pump(2);
	at_check("tap");
}

/* Press and HOLD on a tile for a while — exercises the PRESSED state plus
 * long-press handling, the state the hardware Data Abort fires in. */
static void at_hold(int x, int y, int frames)
{
	gui_logf("autotap: hold (%d,%d) x%d\n", x, y, frames);
	at_press(x, y, true);
	at_pump(frames);
	at_press(x, y, false);
	at_pump(2);
	nav_to_root();
	at_pump(2);
	at_check("hold");
}

/* A real pointer drag: press, walk the finger in steps (LVGL turns this into
 * a scroll on the grid), release. Reproduces the "or while scrolling" path. */
static void at_drag(int x0, int y0, int x1, int y1, int steps)
{
	gui_logf("autotap: drag (%d,%d)->(%d,%d)\n", x0, y0, x1, y1);
	at_press(x0, y0, true);
	at_pump(2);
	for (int i = 1; i <= steps; i++) {
		int x = x0 + (x1 - x0) * i / steps;
		int y = y0 + (y1 - y0) * i / steps;
		at_press(x, y, true);
		at_pump(1);		/* one frame per move → momentum builds */
	}
	at_press(x1, y1, false);
	at_pump(3);
	at_check("drag");
}

static int run_autotap(void)
{
	const int col_x[4] = { 74, 184, 294, 404 };	/* 4-col badge centers */

	gui_log("autotap: begin\n");
	at_pump(4);					/* let the boot splash settle */
	nav_set_root(home_create, NULL);
	nav_show_chrome(true);
	at_pump(4);

	/* 1) Press every visible tile (badge centers ~142 + row*108). */
	for (int row = 0; row < 4; row++)
		for (int c = 0; c < 4; c++)
			at_tap(col_x[c], 142 + row * 108);

	/* 2) Long-hold a few tiles — the exact PRESSED-render state the abort
	 *    hits on first touch. */
	at_hold(74, 142, 8);
	at_hold(184, 250, 8);
	at_hold(294, 358, 8);

	/* 3) Finger-drag the grid up and down repeatedly (scroll render). */
	for (int i = 0; i < 4; i++) {
		at_drag(240, 560, 240, 160, 10);	/* fling up   */
		at_drag(240, 160, 240, 560, 10);	/* fling down */
	}

	/* 3b) FINE scroll sweep — short drags inching the grid through every
	 *     position so a rounded badge is clipped at the top/bottom edge at
	 *     every possible sub-pixel offset (the corrupt-blend-area suspect). */
	for (int rep = 0; rep < 3; rep++) {
		for (int i = 0; i < 40; i++)		/* creep up 40 steps × 16px */
			at_drag(240, 300, 240, 284, 2);
		for (int i = 0; i < 40; i++)		/* creep back down */
			at_drag(240, 284, 240, 300, 2);
	}
	/* 3c) Tiny jittery drags at the grid's top edge — where a badge first
	 *     crosses the clip boundary as it scrolls in/out of view. */
	for (int i = 0; i < 30; i++) {
		at_drag(240, 160, 240, 150, 1);
		at_drag(240, 150, 240, 170, 1);
	}

	/* 4) Programmatic scroll to the bottom, press the lower rows. */
	home_scroll_to_end(1, 0);
	at_pump(6);
	for (int row = 0; row < 4; row++)
		for (int c = 0; c < 4; c++)
			at_tap(col_x[c], 200 + row * 108);

	/* 5) Press-while-scrolling: tap a tile mid-fling so a press lands on a
	 *    moving surface (press-lost + scroll render overlap). */
	for (int i = 0; i < 3; i++) {
		at_press(240, 520, true);
		at_pump(1);
		at_press(240, 320, true);	/* moved → scroll starts */
		at_pump(1);
		at_press(74, 200, true);	/* jump onto a tile */
		at_pump(2);
		at_press(74, 200, false);
		at_pump(3);
		nav_to_root();
		at_pump(2);
	}

	/* 6) Back to the top, then press the floating dock row. */
	home_scroll_to_end(0, 0);
	at_pump(6);
	for (int c = 0; c < 4; c++)
		at_tap(col_x[c], 690);

#ifdef NOTHAN_MEM_TRACE
	{
		extern size_t nothan_mem_biggest;
		gui_logf("autotap: biggest single LVGL alloc = %zu bytes\n",
			 nothan_mem_biggest);
	}
#endif
	gui_log("autotap: done — no host-side over-run detected\n");
	return 0;
}

#ifdef SIM_STACKTEST
/*
 * Stack high-water measurement (`make stacktest`).
 *
 * The OS gives the GUI process NO heap — every render temporary lives on the
 * 128 KB user stack (USER_STACK_PAGES=32 in spawn.c, already bumped once
 * because LVGL 9.5's deep draw chains overran 32 KB). This runs the full
 * press/scroll autotap on a generous pthread stack painted with a sentinel,
 * then scans how deep the render actually reached — so we can compare the
 * real peak against the 128 KB the hardware gives it.
 *
 * Host x86-64 frames are a conservative UPPER bound for ARM (8-byte vs 4-byte
 * pointers/longs), so "fits here" ⇒ "fits on ARM"; "overflows here" ⇒ ARM is
 * at serious risk.
 */
#include <pthread.h>
#include <sys/mman.h>

#define MEAS_SENTINEL  0xA5
static unsigned char *meas_lo;		/* lowest address of the pthread stack */

static void *autotap_stack_thread(void *arg)
{
	(void)arg;
	volatile unsigned char probe;
	unsigned char *sp_now = (unsigned char *)&probe;

	/* Paint everything below the current frame down to the stack base. */
	for (unsigned char *p = meas_lo + 64; p < sp_now - 512; p++)
		*p = MEAS_SENTINEL;

	run_autotap();

	/* Deepest touched = lowest address whose sentinel got overwritten. */
	unsigned char *p = meas_lo + 64;
	while (p < sp_now && *p == MEAS_SENTINEL)
		p++;
	size_t used = (size_t)(sp_now - p);
	gui_logf("STACKTEST: peak render stack >= %zu bytes (%zu KB) "
		 "[hardware gives 128 KB]\n", used, used / 1024);
	if (used > 128u * 1024)
		gui_logf("STACKTEST: *** EXCEEDS 128 KB user stack — target would OVERFLOW ***\n");
	else
		gui_logf("STACKTEST: within 128 KB (%zu KB headroom on host)\n",
			 (128u * 1024 - used) / 1024);
	return NULL;
}

static int run_autotap_measured(void)
{
	const size_t sz = 8u * 1024 * 1024;	/* roomy 8 MB measurement stack */
	pthread_attr_t attr;
	pthread_t th;

	/* Own the stack so we know its exact base (race-free, unlike getstack
	 * when only the size is set). Stack grows DOWN from base+sz. */
	void *stk = mmap(NULL, sz, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (stk == MAP_FAILED) {
		gui_log("STACKTEST: mmap stack failed\n");
		return 1;
	}
	meas_lo = (unsigned char *)stk;

	pthread_attr_init(&attr);
	pthread_attr_setstack(&attr, stk, sz);
	if (pthread_create(&th, &attr, autotap_stack_thread, NULL) != 0) {
		gui_log("STACKTEST: pthread_create failed\n");
		return 1;
	}
	pthread_join(th, NULL);
	pthread_attr_destroy(&attr);
	return 0;
}
#endif /* SIM_STACKTEST */
#endif /* SIM_AUTOTAP */

int main(void)
{
#ifndef SIM_HEADLESS
	SDL_Init(SDL_INIT_VIDEO);
#endif

	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	nav_init();
	contacts_init();
	messages_init();
	telephony_init();
	call_ui_init();
	nav_set_root(boot_create, NULL);
	gui_log("ready\n");

#ifdef SIM_STACKTEST
	return run_autotap_measured();
#elif defined(SIM_AUTOTAP)
	return run_autotap();
#endif

#ifndef SIM_HEADLESS
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
#else
	return 0;	/* SIM_HEADLESS: autotap path already returned above */
#endif
}
