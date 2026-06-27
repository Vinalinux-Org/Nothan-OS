/*
 * core/monkey.c - On-target soak-test input generator (build: make MONKEY=1)
 *
 * Feeds the LVGL pointer indev a stream of randomized but seeded user gestures
 * focused on the DEMO surface: scrolling the home grid up and down, opening the
 * three default apps (Phone / Messages / Contacts) from the dock, using their
 * lists, and backing out again. The push/pop screen stack and the scroll path
 * — the two render paths that have crashed on hardware — get hammered. The
 * point is to exercise the demo for hours on the real target build and surface
 * intermittent faults that no host build reproduces.
 *
 * Reproducible: the seed is printed at start and each gesture is logged with a
 * step counter, so a crash's UART tail shows the exact sequence and the same
 * seed replays it bit-for-bit.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#ifdef GUI_MONKEY

#include "lvgl/lvgl.h"
#include "monkey.h"
#include "log.h"

/* Logical portrait area the gestures address; LVGL rotates to the panel. */
#define LX_MAX  480
#define LY_MAX  800

/*
 * Demo launch targets in logical coords. The dock is the always-visible bottom
 * tray (Phone / Messages / Contacts in columns 0..2 of the 92%-wide 4-col
 * grid); the grid scrolls vertically between the search bar and the dock; the
 * nav bar sits below the dock; the in-app back chevron is top-left.
 */
static const int APP_X[3] = { 74, 184, 294 };	/* Phone, Messages, Contacts */
#define DOCK_Y     688				/* floating dock row */
#define NAVBAR_Y   772				/* system nav bar (home/back) */
#define GRID_TOP   120				/* scrollable grid/list top */
#define GRID_BOT   630				/* ...and bottom */

/* xorshift32 — tiny, deterministic, good enough to spray the UI. */
static uint32_t rng;
static uint32_t rng_next(void)
{
	uint32_t x = rng;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return rng = x;
}

static int rrange(int lo, int hi)	/* inclusive both ends */
{
	return lo + (int)(rng_next() % (uint32_t)(hi - lo + 1));
}

enum { G_IDLE, G_TAP, G_HOLD, G_DRAG };

static int g_kind = G_IDLE;
static int g_frame, g_frames;		/* sample index / length of current gesture */
static int g_x0, g_y0, g_x1, g_y1;	/* logical endpoints */
static int want_idle;			/* settle one gesture, act the next */
static unsigned g_step;			/* count of real gestures emitted */

/* Choose the next gesture. Every other call is a short idle so LVGL processes
 * the release (→ CLICKED → nav) before the next press lands. */
static void pick(void)
{
	g_frame = 0;

	if (want_idle) {
		g_kind = G_IDLE;
		g_frames = rrange(2, 4);
		want_idle = 0;
		return;
	}
	want_idle = 1;
	g_step++;

	static const char *appnm[3] = { "Phone", "Messages", "Contacts" };
	int r = rrange(0, 99);
	if (r < 34) {				/* scroll the home grid / app list up-down */
		g_kind = G_DRAG; g_frames = rrange(6, 12);
		g_x0 = g_x1 = rrange(80, 400);
		if (rng_next() & 1) { g_y0 = rrange(520, GRID_BOT); g_y1 = rrange(GRID_TOP, 300); }
		else                { g_y0 = rrange(GRID_TOP, 300); g_y1 = rrange(520, GRID_BOT); }
		gui_logf("[MONKEY] #%u scroll  (%d,%d)->(%d,%d)\n",
			 g_step, g_x0, g_y0, g_x1, g_y1);
	} else if (r < 64) {			/* open a demo app from the dock */
		int a = rrange(0, 2);
		g_kind = G_TAP; g_frames = 3;
		g_x0 = APP_X[a] + rrange(-16, 16); g_y0 = DOCK_Y + rrange(-12, 12);
		gui_logf("[MONKEY] #%u open-%s (%d,%d)\n", g_step, appnm[a], g_x0, g_y0);
	} else if (r < 82) {			/* in-app back chevron (top-left) */
		g_kind = G_TAP; g_frames = 3;
		g_x0 = rrange(8, 56); g_y0 = rrange(24, 72);
		gui_logf("[MONKEY] #%u back    (%d,%d)\n", g_step, g_x0, g_y0);
	} else if (r < 92) {			/* nav bar → jump home */
		g_kind = G_TAP; g_frames = 3;
		g_x0 = rrange(40, 440); g_y0 = NAVBAR_Y + rrange(-12, 12);
		gui_logf("[MONKEY] #%u navhome (%d,%d)\n", g_step, g_x0, g_y0);
	} else {				/* tap a list row / tile inside the app */
		g_kind = G_TAP; g_frames = 3;
		g_x0 = rrange(40, 440); g_y0 = rrange(GRID_TOP, 600);
		gui_logf("[MONKEY] #%u tap     (%d,%d)\n", g_step, g_x0, g_y0);
	}

	if ((g_step & 0xFF) == 0) {		/* heartbeat: survival + heap health */
		lv_mem_monitor_t m;
		lv_mem_monitor(&m);
		gui_logf("[MONKEY] === %u gestures alive, heap used=%u%% free=%uB frag=%u%% ===\n",
			 g_step, (unsigned)m.used_pct,
			 (unsigned)m.free_size, (unsigned)m.frag_pct);
	}
}

void monkey_init(uint32_t seed)
{
	rng = seed ? seed : 0xA5A5F00Du;
	g_kind = G_IDLE; g_frame = 0; g_frames = 6;
	want_idle = 0; g_step = 0;
	gui_logf("[MONKEY] begin seed=0x%x (tap/hold/scroll/swipe/back/nav)\n",
		 (unsigned)rng);
}

void monkey_read(int *lx, int *ly, int *pressed)
{
	if (g_frame >= g_frames)
		pick();

	int last = (g_frame >= g_frames - 1);	/* final sample releases */
	int x = g_x0, y = g_y0, p = 0;

	switch (g_kind) {
	case G_TAP:
	case G_HOLD:
		p = !last; x = g_x0; y = g_y0;
		break;
	case G_DRAG: {
		int span = g_frames > 1 ? g_frames - 1 : 1;
		x = g_x0 + (g_x1 - g_x0) * g_frame / span;
		y = g_y0 + (g_y1 - g_y0) * g_frame / span;
		p = !last;
		break;
	}
	default:				/* G_IDLE */
		p = 0; x = g_x0; y = g_y0;
		break;
	}
	g_frame++;

	*lx = x; *ly = y; *pressed = p;
}

#endif /* GUI_MONKEY */
