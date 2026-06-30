/*
 * core/monkey.c - Deterministic random touch input generator
 *
 * Produces a repeatable gesture stream (tap, hold, drag) seeded from a
 * fixed value so host (ASan) and target runs are bit-for-bit identical.
 * Called once per frame by the sim autotap; monkey_read() yields the
 * (lx, ly, pressed) state for that frame in logical display coordinates.
 *
 * Display logical size: 360 x 640.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "monkey.h"

#define DISP_W  360
#define DISP_H  640

/* xorshift32 PRNG */
static uint32_t s_seed;

static uint32_t rng(void)
{
	s_seed ^= s_seed << 13;
	s_seed ^= s_seed >> 17;
	s_seed ^= s_seed << 5;
	return s_seed;
}

static int rng_range(int lo, int hi)
{
	return lo + (int)(rng() % (uint32_t)(hi - lo + 1));
}

/* Gesture state machine */
typedef enum {
	ST_IDLE,	/* finger up, waiting N frames before next gesture */
	ST_TAP,		/* finger down at (cx, cy), stays N frames */
	ST_DRAG_DN,	/* drag: finger moving toward (tx, ty), still pressing */
	ST_DRAG_UP,	/* finger lifted after drag, 1..2 cool-down frames */
} state_t;

static state_t s_state;
static int s_cx, s_cy;		/* current finger position */
static int s_tx, s_ty;		/* drag target */
static int s_frames;		/* frames remaining in current phase */
static int s_drag_step;		/* current drag step */
static int s_drag_steps;	/* total drag steps */

static void start_idle(void)
{
	s_state  = ST_IDLE;
	s_frames = rng_range(1, 5);
}

static void start_tap(void)
{
	s_cx     = rng_range(10, DISP_W - 10);
	s_cy     = rng_range(10, DISP_H - 10);
	s_state  = ST_TAP;
	s_frames = rng_range(2, 18);
}

static void start_drag(void)
{
	s_cx        = rng_range(10, DISP_W - 10);
	s_cy        = rng_range(10, DISP_H - 10);
	s_tx        = rng_range(10, DISP_W - 10);
	s_ty        = rng_range(10, DISP_H - 10);
	s_drag_step  = 0;
	s_drag_steps = rng_range(6, 24);
	s_state      = ST_DRAG_DN;
}

void monkey_init(uint32_t seed)
{
	s_seed = seed ? seed : 1;
	start_idle();
}

void monkey_read(int *lx, int *ly, int *pressed)
{
	switch (s_state) {
	case ST_IDLE:
		*lx = s_cx;
		*ly = s_cy;
		*pressed = 0;
		if (--s_frames <= 0) {
			/* 30% drag, 70% tap */
			if (rng_range(0, 9) < 3)
				start_drag();
			else
				start_tap();
		}
		break;

	case ST_TAP:
		*lx = s_cx;
		*ly = s_cy;
		*pressed = 1;
		if (--s_frames <= 0)
			start_idle();
		break;

	case ST_DRAG_DN:
		s_drag_step++;
		s_cx = s_cx + (s_tx - s_cx) * s_drag_step / s_drag_steps;
		s_cy = s_cy + (s_ty - s_cy) * s_drag_step / s_drag_steps;
		*lx = s_cx;
		*ly = s_cy;
		*pressed = 1;
		if (s_drag_step >= s_drag_steps) {
			s_state  = ST_DRAG_UP;
			s_frames = rng_range(1, 3);
		}
		break;

	case ST_DRAG_UP:
		*lx = s_cx;
		*ly = s_cy;
		*pressed = 0;
		if (--s_frames <= 0)
			start_idle();
		break;
	}
}
