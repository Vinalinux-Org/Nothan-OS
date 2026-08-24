/*
 * drivers/video/vsource-test.c - a camera that is not there yet
 *
 * Stands in for the USB webcam so that everything downstream of the capture
 * seam — framing, transmission, reassembly, display — can be written, run and
 * measured before the webcam exists.  When the UVC driver lands it registers
 * through the same seam and this file stops being built; nothing above it
 * changes, which is the whole reason the seam was drawn first.
 *
 * The pattern is one add per pixel: value = x + y + seq, taken raw as RGB565.
 * That gives diagonal bands which slide by one pixel a frame, so motion is
 * obvious on a screen, and it is an exact formula, so a receiver can check
 * every pixel it was given rather than squinting at a picture.  Cheap matters:
 * a real camera writes its frames by DMA and costs the CPU nothing, so every
 * cycle spent generating here is a cycle the measurements will over-report.
 * At 400x240 and thirty frames a second this is about half a per cent.
 *
 * One buffer, not two.  A camera needs a second so the hardware can fill one
 * while the reader holds the other; this one generates on demand inside
 * capture(), so a second buffer would sit empty.  Buffer count is behind the
 * seam and no consumer can tell.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/video.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/delay.h>
#include <nothan/printk.h>
#include <nothan/config.h>

/*
 * 400x240 is half of the 800x480 panel in each direction, which means it
 * reaches the screen by doubling pixels rather than by interpolating between
 * them — no filter, no arithmetic per output pixel, and no blur.  At thirty
 * frames a second it is 48.4 Mbit/s on the wire and about 13.5% of the CPU for
 * both directions of a call, against 96.3 Mbit/s for the full panel at fifteen
 * frames, which the link cannot carry alongside anything else.
 */
#define VSRC_TEST_W		400
#define VSRC_TEST_H		240
#define VSRC_TEST_FPS		30

#define FRAME_BYTES		(VSRC_TEST_W * VSRC_TEST_H * VIDEO_BYTES_PER_PIXEL)

static u8 vsrc_test_pixels[FRAME_BYTES];

static struct video_frame vsrc_test_frame = {
	.width  = VSRC_TEST_W,
	.height = VSRC_TEST_H,
	.pixels = vsrc_test_pixels,
};

/*
 * When the next frame is due, in jiffies.
 *
 * Kept as a deadline rather than as a delay so the rate cannot drift: sleeping
 * a fixed interval after finishing each frame makes the period the interval
 * plus however long the work took, which is how a thirty frame source quietly
 * becomes a twenty-six frame one.
 */
static unsigned long vsrc_test_due;

static unsigned long vsrc_test_late;	/* deadlines already past on arrival */

/*
 * The number of the frame about to be generated.
 *
 * Held apart from vsrc_test_frame.seq, which is the number of the frame
 * currently in the buffer, because the two are not the same thing and treating
 * them as one produced the first bug this pattern caught.  Generating with the
 * counter and incrementing afterwards handed the caller a frame whose pixels
 * were drawn from N while its seq field said N+1 — every pixel correct, every
 * frame mislabelled, and nothing but a byte-for-byte check could see it.
 */
static u32 vsrc_test_next;

static struct video_frame *vsrc_test_capture(struct video_source *src)
{
	u16 *p = (u16 *)vsrc_test_pixels;
	unsigned int x, y;
	unsigned long now;
	u32 seq = vsrc_test_next++;

	(void)src;

	now = get_jiffies();
	if (!vsrc_test_due)
		vsrc_test_due = now;

	if ((long)(vsrc_test_due - now) > 0) {
		msleep(vsrc_test_due - now);
	} else if (vsrc_test_due != now) {
		vsrc_test_late++;

		/*
		 * More than a whole frame behind, so give up on the frames that
		 * were missed instead of running flat out to catch them.  Left
		 * unhandled this is a source that falls behind once and then
		 * never sleeps again, which looks like a capture rate that
		 * suddenly doubled — a stand-in generating as fast as the CPU
		 * allows, reported as thirty frames a second.
		 */
		if ((unsigned long)(now - vsrc_test_due) > 1000UL / VSRC_TEST_FPS)
			vsrc_test_due = now;
	}

	/*
	 * 1000 / 30 is 33.33, and a tick is a whole millisecond.  Advancing by
	 * the rounded interval would run 1% fast; carrying the remainder in the
	 * deadline itself keeps the long-run rate exact without needing a finer
	 * clock than the one the box has.
	 */
	vsrc_test_due += 1000UL / VSRC_TEST_FPS;
	if ((seq % 3) == 0)
		vsrc_test_due++;		/* 34, 33, 33, 34, ... */

	for (y = 0; y < VSRC_TEST_H; y++) {
		u16 v = (u16)(y + seq);

		for (x = 0; x < VSRC_TEST_W; x++)
			*p++ = (u16)(v + x);
	}

	vsrc_test_frame.seq = seq;
	return &vsrc_test_frame;
}

static void vsrc_test_release(struct video_source *src)
{
	(void)src;	/* nothing to give back: the buffer is refilled in place */
}

static struct video_source vsrc_test = {
	.name    = "testpattern",
	.width   = VSRC_TEST_W,
	.height  = VSRC_TEST_H,
	.capture = vsrc_test_capture,
	.release = vsrc_test_release,
};

static int __init vsrc_test_init(void)
{
	if (!CONFIG_VIDEO_TESTSRC)
		return 0;

	return video_source_register(&vsrc_test);
}
device_initcall(vsrc_test_init);
