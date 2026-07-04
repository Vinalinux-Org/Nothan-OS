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
#include "screens/call_log.h"
#include "screens/sms_list.h"
#include "screens/sms_chat.h"
#include "screens/contacts_list.h"
#include "screens/contact_detail.h"
#include "screens/contacts_add.h"
#include "screens/dialer.h"
#include "core/call_ui.h"
#include "services/contacts.h"
#include "services/messages.h"
#include "services/telephony.h"
#include "services/modem_client.h"

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
		modem_pump();		/* dispatch URCs before render, like real main loop */
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

/* A bare press/release with NO nav_to_root afterwards: whatever the touched
 * widget's CLICKED handler does (nav_push, nav_pop, …) is left in place. This
 * is how the on-screen back button is exercised — its handler deletes the
 * current screen from inside its own event, the hardware use-after-free. */
static void at_click(int x, int y)
{
	gui_logf("autotap: click (%d,%d)\n", x, y);
	at_press(x, y, true);
	at_pump(3);
	at_press(x, y, false);
	at_pump(3);
	at_check("click");
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

/* =======================================================================
 * Per-screen autotap sequences (logical display 480×800 after ROTATION_270)
 *
 * Each function nav_push()es its target screen, exercises the key paths
 * (scroll, tap, back), then returns to home via nav_to_root(). The same
 * at_check() after each screen catches any heap corruption the interaction
 * triggers. Coordinates are approximate — close enough to hit the widget.
 *
 * List row geometry: rows are ~100 px tall, content starts at y≈120.
 *   AT_ROW(0)=170, AT_ROW(1)=270, AT_ROW(2)=370, AT_ROW(3)=470
 * =======================================================================*/

/* Entry points declared in telephony.c / messages.c — called by modem_client */
extern void telephony_on_incoming(const char *number);
extern void sms_on_received(const char *peer, const char *text);

/* Seed realistic test data so every screen has something to render and
 * scroll. Called once at the top of run_autotap(), before screen tests. */
static void at_seed_data(void)
{
	/* Contacts */
	contacts_add("An Nguyen",    "0868 000 001");
	contacts_add("Binh Tran",    "0912 345 678");
	contacts_add("Cuong Le",     "0987 654 321");
	contacts_add("Dung Pham",    "0901 111 222");
	contacts_add("Giang Hoang",  "0933 444 555");

	/* SMS conversations — a few messages each */
	sms_on_received("0868 000 001", "Hey, are you free?");
	sms_on_received("0868 000 001", "Let me know when you arrive");
	sms_on_received("0912 345 678", "Meeting moved to 3pm");
	sms_on_received("0912 345 678", "Can you confirm?");
	sms_on_received("0987 654 321", "On my way");
}

#define AT_CX        240		/* center X of the 480-wide display     */
#define AT_BACK_X    43		/* header back-chevron position          */
#define AT_BACK_Y    54
#define AT_ROW(n)   (170 + (n) * 100)	/* list-row center Y             */

/* Dial-pad column centers (3 cols across 480 px) and row centers */
#define AT_DPAD_C0   80
#define AT_DPAD_C1   240
#define AT_DPAD_C2   400
#define AT_DPAD_R(n) (220 + (n) * 100)  /* rows 0-3: 1-9 + *0#           */

/* Push a screen, let it settle, then exercise it; always land back on home. */
static void at_screen_open(nav_builder_fn b, void *arg, const char *name)
{
	gui_logf("autotap: >> %s\n", name);
	nav_push(b, arg);
	at_pump(5);
}

static void at_screen_back(void)
{
	at_click(AT_BACK_X, AT_BACK_Y);
	at_pump(4);
	nav_to_root();
	at_pump(2);
}

static void at_test_call_log(void)
{
	at_screen_open(call_log_create, NULL, "call_log");
	at_drag(AT_CX, AT_ROW(3), AT_CX, AT_ROW(0), 8);	/* scroll down */
	at_drag(AT_CX, AT_ROW(0), AT_CX, AT_ROW(3), 8);	/* scroll up   */
	at_tap(AT_CX, AT_ROW(0));	/* tap first row → dial (no-op in sim) */
	at_tap(AT_CX, AT_ROW(1));
	/* open dialer from call_log keypad icon (top-right, ~x=440, y=54) */
	at_click(440, 54);
	at_pump(4);
	nav_to_root();
	at_pump(2);
	at_check("call_log");
}

static void at_test_dialer(void)
{
	at_screen_open(dialer_create, NULL, "dialer");
	/* tap digits: 1 2 3 4 5 6 0 */
	at_tap(AT_DPAD_C0, AT_DPAD_R(0));	/* 1 */
	at_tap(AT_DPAD_C1, AT_DPAD_R(0));	/* 2 */
	at_tap(AT_DPAD_C2, AT_DPAD_R(0));	/* 3 */
	at_tap(AT_DPAD_C0, AT_DPAD_R(1));	/* 4 */
	at_tap(AT_DPAD_C1, AT_DPAD_R(1));	/* 5 */
	at_tap(AT_DPAD_C2, AT_DPAD_R(1));	/* 6 */
	at_tap(AT_DPAD_C1, AT_DPAD_R(3));	/* 0 */
	/* backspace twice */
	at_tap(380, 570);
	at_tap(380, 570);
	/* call button (no-op: modem_cmd_dial stub) */
	at_tap(AT_CX, 570);
	at_pump(4);
	nav_to_root();
	at_pump(2);
	at_check("dialer");
}

static void at_test_sms_list(void)
{
	at_screen_open(sms_list_create, NULL, "sms_list");
	at_drag(AT_CX, AT_ROW(3), AT_CX, AT_ROW(0), 8);
	at_drag(AT_CX, AT_ROW(0), AT_CX, AT_ROW(3), 8);
	/* open first conversation if one exists */
	if (sms_conversation_count() > 0) {
		at_click(AT_CX, AT_ROW(0));
		at_pump(5);
		/* inside sms_chat: scroll thread */
		at_drag(AT_CX, AT_ROW(2), AT_CX, AT_ROW(0), 6);
		at_drag(AT_CX, AT_ROW(0), AT_CX, AT_ROW(2), 6);
		/* tap send button area (keyboard not open, so it's a no-op) */
		at_tap(AT_CX, 700);
		at_pump(2);
		at_click(AT_BACK_X, AT_BACK_Y);	/* back from sms_chat */
		at_pump(4);
	}
	at_screen_back();
	at_check("sms_list");
}

static void at_test_contacts_list(void)
{
	at_screen_open(contacts_list_create, NULL, "contacts_list");
	at_drag(AT_CX, AT_ROW(3), AT_CX, AT_ROW(0), 8);
	at_drag(AT_CX, AT_ROW(0), AT_CX, AT_ROW(3), 8);
	if (contacts_count() > 0) {
		/* open first contact → contact_detail */
		at_click(AT_CX, AT_ROW(0));
		at_pump(5);
		/* inside contact_detail: tap Call and SMS action buttons */
		at_tap(160, 520);	/* Call button */
		at_pump(3);
		at_tap(320, 520);	/* SMS button */
		at_pump(3);
		nav_to_root();
		at_pump(2);
		/* re-enter contacts to test Add flow */
		nav_push(contacts_list_create, NULL);
		at_pump(4);
	}
	/* tap Add button (top-right, ~x=440, y=54) */
	at_click(440, 54);
	at_pump(5);
	at_screen_back();	/* back from contacts_add without saving */
	at_screen_back();	/* back from contacts_list */
	at_check("contacts_list");
}

static void at_test_contacts_add(void)
{
	/* Add with empty name → Save should be a no-op (guard in app code) */
	at_screen_open(contacts_add_create, NULL, "contacts_add");
	at_tap(AT_CX, 650);	/* Save button */
	at_pump(2);
	at_screen_back();
	at_check("contacts_add");
}

static void at_test_call_overlay(void)
{
	gui_log("autotap: >> call_overlay\n");

	/* Incoming call → reject */
	telephony_on_incoming("0868 000 000");
	at_pump(5);
	at_tap(140, 620);	/* reject button */
	at_pump(5);
	at_check("overlay_reject");

	/* Incoming call → accept → hang up */
	telephony_on_incoming("0912 345 678");
	at_pump(5);
	at_tap(340, 620);	/* accept button */
	at_pump(5);
	at_tap(AT_CX, 620);	/* hang up */
	at_pump(8);
	at_check("overlay_accept_hangup");
}

static int run_autotap(void)
{
	const int col_x[4] = { 74, 184, 294, 404 };	/* 4-col badge centers */

	gui_log("autotap: begin\n");
	at_seed_data();
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

	/* 7) Open each app from the dock, then leave via the on-screen header
	 *    BACK chevron (logical ~43,54) — a real CLICKED event whose handler
	 *    calls nav_pop(), deleting the screen that owns the chevron from
	 *    inside its own event. Repeat across screens: this is the exact
	 *    hardware Data Abort (get_prop_core walking a freed obj's styles). */
	for (int rep = 0; rep < 4; rep++) {
		for (int c = 0; c < 4; c++) {
			at_click(col_x[c], 695);	/* dock tile → nav_push */
			at_click(43, 54);		/* header back → nav_pop (in-event) */
		}
	}

	/* 8) Per-screen deep tests — exercise every app screen systematically. */
	at_test_call_log();
	at_test_dialer();
	at_test_sms_list();
	at_test_contacts_list();
	at_test_contacts_add();
	at_test_call_overlay();

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

#ifdef SIM_MONKEY
#include "core/monkey.h"
#include "services/telephony.h"
#include "services/messages.h"

/*
 * Drive the REAL target monkey generator (gui/core/monkey.c, same seed) through
 * the sim indev under ASan. The target build crashes deterministically at a
 * given gesture #; if the same gesture stream trips ASan here, the fault is an
 * allocator-independent LOGIC bug (use-after-free / overflow) with a full host
 * backtrace. If the host sails past it clean, the fault is target-only codegen
 * (the arm-none-eabi -O2 miscompile) — exactly what -O1 fixes.
 *
 * monkey_read() yields one input sample per call, like the on-target indev read
 * cb; we call it once per frame so the gesture sequence matches bit-for-bit.
 */
#ifndef SIM_MONKEY_FRAMES
#define SIM_MONKEY_FRAMES  6000
#endif
static uint32_t get_seed(void)
{
	const char *s = getenv("MONKEY_SEED");
	if (s && *s)
		return (uint32_t)strtoul(s, NULL, 0);
	return 0x1234abcd;
}

static int run_monkey(void)
{
	uint32_t seed = get_seed();
	gui_logf("monkey-sim: begin seed=0x%08x\n", seed);
	monkey_init(seed);

	at_pump(4);
	nav_set_root(home_create, NULL);
	nav_show_chrome(true);
#ifdef MONKEY_MOCK
	telephony_set_mock(1);			/* mirror the GUI_MONKEY build */
	messages_set_mock(1);
#else
	telephony_set_mock(0);			/* demo-faithful: no async radio events */
	messages_set_mock(0);
#endif
	telephony_calllog_clear();		/* identical clean start as the target */
	at_pump(4);

	for (int f = 0; f < SIM_MONKEY_FRAMES; f++) {
		int lx, ly, pressed;
		monkey_read(&lx, &ly, &pressed);
		at_press(lx, ly, pressed);
		at_pump(1);
		if ((f & 31) == 0)
			at_check("monkey");
	}
	gui_log("monkey-sim: done — no host-side fault over the gesture stream\n");
	return 0;
}
#endif /* SIM_MONKEY */

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

#ifdef SIM_MONKEY
	run_monkey();		/* measure the monkey's deep paths, not just the scripted sweep */
#else
	run_autotap();
#endif

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
	return run_autotap_measured();	/* wraps run_monkey/run_autotap on a measured stack */
#elif defined(SIM_MONKEY)
	return run_monkey();
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
