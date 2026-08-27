/*
 * net/video_rx.c - put the far end's frames back together and on the screen
 *
 * The receiving half of a call, and the other end of net/video_tx.c.  A frame
 * arrives as about 132 datagrams, each saying where in the frame it belongs, so
 * reassembly is a write at an offset: no queue, no sort, and no dependency
 * between one datagram and the next.
 *
 * Its own socket and its own port, which is not a style choice.  ring.h is lock
 * free on the strength of one producer and one consumer, and two tasks reading
 * one socket is exactly the second consumer that contract rules out.  The
 * sender's socket carries control messages to the sender's task; this one
 * carries pixels to this one.
 *
 * Measured, both directions at once against a laptop: 600 frames of 400x240 at
 * 30.0 a second, every one of them whole, 79,596 datagrams with none dropped by
 * any ring and none refused by the MAC, and 73% of the machine still idle while
 * the raster DMA pulled 46 MB/s out of DDR underneath it.  That last part was
 * the open question when CONFIG_VIDEO went on — whether the display stealing
 * DDR bandwidth would cost the network anything.  It costs nothing measurable.
 *
 * A missing datagram leaves a hole, and the hole shows whatever was in the
 * staging buffer before — which is the same region of an earlier frame.  That
 * is deliberate and it is also free: not clearing the buffer between frames is
 * the whole of the implementation.  Dropping the frame instead would trade a
 * stale band lasting 33 milliseconds for a visible stutter, and on a stream
 * with no back channel the stutter cannot be made up later.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/video.h>
#include <nothan/video_net.h>
#include <nothan/fb.h>
#include <nothan/sched.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/printk.h>

void uvc_dump_state(void);
void lcdc_dump_health(void);

#if CONFIG_VIDEO_STREAM

/*
 * The largest frame this will assemble.  Sized for the geometry the sender
 * uses rather than for the panel: a frame is doubled on its way to the screen,
 * so half the panel in each direction is the whole of what can arrive.
 */
#define VRX_MAX_W		400
#define VRX_MAX_H		240
#define VRX_MAX_BYTES		(VRX_MAX_W * VRX_MAX_H * VIDEO_BYTES_PER_PIXEL)

static struct udp_sock vrx_sock;

/*
 * Aligned so the doubling loop below can read pairs of pixels as words.  Not
 * cleared at startup either: the first frames of a stream draw over whatever
 * the kernel left here, which is visible for a thirtieth of a second and
 * cheaper than zeroing 192 KB that is about to be overwritten.
 */
static u8 vrx_stage[VRX_MAX_BYTES] __attribute__((aligned(4)));

DEFINE_RATELIMIT(vrx_rl, 1000, 1);

/*
 * The two probes into other subsystems, once every ten seconds rather than
 * every one.
 *
 * They were written to find a bug that turned out not to exist — the display
 * path was correct throughout, and what was wrong was the picture being sent
 * to it — so what is left is telemetry rather than an investigation.  Useful
 * telemetry: an LCDC underflow count and the address the DMA is actually
 * reading are the two facts that separate "the write never happened" from
 * "nobody is looking at where it happened", which no amount of frame counting
 * can.  But a line a second forever is how a console stops being readable,
 * and this file has complained about that three times already.
 */
DEFINE_RATELIMIT(vrx_probe_rl, 10000, 1);

static unsigned long vrx_frames;	/* frames finished, whole or holed */
static unsigned long vrx_complete;	/* finished with nothing missing */
static unsigned long vrx_chunks;
static unsigned long vrx_bad;		/* not this protocol, or not sane */
static unsigned long vrx_late;		/* a chunk for a frame already drawn */
static unsigned long vrx_painted;

static u16 be16(const u8 *p) { return (u16)(((u16)p[0] << 8) | p[1]); }

static u32 be32(const u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
	       ((u32)p[2] << 8)  |  (u32)p[3];
}

/*
 * Double a frame into the middle of the panel.
 *
 * Every output pixel is a copy of an input pixel — no filter, no interpolation,
 * no arithmetic per pixel — because 400x240 is exactly half of 800x480 in each
 * direction.  That is why the sender uses that geometry, and choosing anything
 * else turns this loop into a resampler.
 *
 * Two pixels are written as one word, which halves the number of stores, and
 * the second output row is copied from the first rather than recomputed.  Both
 * are safe because the staging buffer and the surface are word aligned and the
 * width is even.
 */
static void vrx_paint(const struct fb_surface *fb, unsigned int sw,
		      unsigned int sh)
{
	const u16 *src = (const u16 *)vrx_stage;
	unsigned int x0 = (fb->width  - sw * 2u) / 2u;
	unsigned int y0 = (fb->height - sh * 2u) / 2u;
	unsigned int x, y;

	for (y = 0; y < sh; y++) {
		u16 *row0 = fb->pixels + (y0 + y * 2u) * fb->width + x0;
		u32 *w0   = (u32 *)row0;
		u32 *w1   = (u32 *)(row0 + fb->width);

		for (x = 0; x < sw; x++) {
			u32 p = src[y * sw + x];

			w0[x] = (p << 16) | p;
		}

		for (x = 0; x < sw; x++)
			w1[x] = w0[x];
	}

	vrx_painted++;
}

/* One frame is over: count whether it was whole, then draw it either way. */
static void vrx_finish(const struct fb_surface *fb, unsigned int w,
		       unsigned int h, unsigned int have)
{
	vrx_frames++;
	if (have == w * h * VIDEO_BYTES_PER_PIXEL)
		vrx_complete++;

	vrx_paint(fb, w, h);
	fb_sync();
}

/*
 * Paint something before any frame arrives.
 *
 * A panel showing black is the same picture whether the display path is
 * broken, the network is silent, or the framebuffer was simply never written —
 * and an allocated framebuffer is zeroed, which in RGB565 is black.  Three
 * causes, one symptom, and no way to tell them apart by looking.
 *
 * So the receiver signs its work on startup.  What it draws is chosen to be
 * read rather than admired: a white frame around the panel says the geometry
 * and stride are right, and four colour bars say the pixel format is — red
 * appearing where blue is expected means the RGB565 byte order is reversed,
 * which is otherwise a thing one discovers much later and blames on the
 * camera.
 */
static void vrx_splash(const struct fb_surface *fb)
{
	static const u16 bar[4] = {
		0xF800,		/* red   */
		0x07E0,		/* green */
		0x001F,		/* blue  */
		0xFFFF,		/* white */
	};
	unsigned int x, y;

	for (y = 0; y < fb->height; y++) {
		u16 *row = fb->pixels + y * fb->width;
		int edge_y = (y < 2 || y >= fb->height - 2);

		for (x = 0; x < fb->width; x++) {
			if (edge_y || x < 2 || x >= fb->width - 2)
				row[x] = 0xFFFF;
			else if (y > fb->height / 3 && y < fb->height * 2 / 3)
				row[x] = bar[(x * 4) / fb->width];
			else
				row[x] = 0x2104;	/* dark grey, not black */
		}
	}

	fb_sync();
}

static void video_rx_task(void)
{
	struct fb_surface fb;
	unsigned int cur_w = 0, cur_h = 0, have = 0;
	u32 cur_seq = 0;
	int started = 0, assembling = 0;
	unsigned long mark_j = 0, mark_f = 0, mark_i = 0;

	if (fb_get_surface(&fb) != 0) {
		printk("[VRX] no display registered; not receiving\n");
		return;
	}

	vrx_splash(&fb);

	printk("[VRX] port %lu, panel %lux%lu, up to %lux%lu doubled\n",
	       (unsigned long)VIDEO_PORT_RX,
	       (unsigned long)fb.width, (unsigned long)fb.height,
	       (unsigned long)VRX_MAX_W, (unsigned long)VRX_MAX_H);

	for (;;) {
		struct udp_datagram *dg = udp_recv(&vrx_sock);
		const u8 *p = dg->data;
		unsigned int off, n, w, h, want;
		u32 seq;
		u8 flags;
		int last;

		if (dg->len <= VIDEO_HDR_LEN ||
		    be16(p + VIDEO_H_MAGIC) != VIDEO_MAGIC ||
		    p[VIDEO_H_VERSION] != VIDEO_VERSION) {
			vrx_bad++;
			udp_done(&vrx_sock);
			continue;
		}

		flags = p[VIDEO_H_FLAGS];
		seq   = be32(p + VIDEO_H_SEQ);
		off   = be32(p + VIDEO_H_OFFSET);
		w     = be16(p + VIDEO_H_WIDTH);
		h     = be16(p + VIDEO_H_HEIGHT);
		n     = dg->len - VIDEO_HDR_LEN;
		last  = (flags & VIDEO_F_LAST) != 0;

		/*
		 * Everything the header claims is checked before it is used.  A
		 * width of zero divides by nothing, an odd width breaks the
		 * word-pair doubling, and an offset near the top of a u32 plus
		 * a length is the classic way a length field becomes a write
		 * past the end of a buffer — so the sum is bounded, not the
		 * parts.
		 */
		want = w * h * VIDEO_BYTES_PER_PIXEL;
		if (!w || !h || (w & 1) || w > VRX_MAX_W || h > VRX_MAX_H ||
		    off > VRX_MAX_BYTES || n > VRX_MAX_BYTES - off ||
		    off + n > want) {
			vrx_bad++;
			udp_done(&vrx_sock);
			continue;
		}

		vrx_chunks++;

		/*
		 * A straggler for a frame already on the screen.  Writing it
		 * would put a band of the past into the present, and starting a
		 * fresh frame from it would undo a newer one — so it is dropped,
		 * whether it is older than the current frame or a leftover of
		 * the current frame after its last chunk already arrived.
		 *
		 * Signed subtraction, so the comparison keeps working when the
		 * sender's counter wraps.
		 */
		if (started && ((long)(seq - cur_seq) < 0 ||
				(seq == cur_seq && !assembling))) {
			vrx_late++;
			udp_done(&vrx_sock);
			continue;
		}

		/*
		 * A datagram from a newer frame ends the one in progress,
		 * whether or not its last chunk ever arrived.  That fallback is
		 * why VIDEO_F_LAST can be a hint rather than a promise: the one
		 * datagram whose loss cannot be inferred from anything else is
		 * the one that says the frame is over.
		 */
		if (seq != cur_seq || !started) {
			if (assembling)
				vrx_finish(&fb, cur_w, cur_h, have);

			started    = 1;
			assembling = 1;
			cur_seq = seq;
			cur_w = w;
			cur_h = h;
			have = 0;
		}

		{
			u8 *dst = vrx_stage + off;
			unsigned int i;

			for (i = 0; i < n; i++)
				dst[i] = p[VIDEO_HDR_LEN + i];
		}
		have += n;

		udp_done(&vrx_sock);

		if (last) {
			vrx_finish(&fb, cur_w, cur_h, have);
			assembling = 0;
		}

		if (ratelimit_allow(&vrx_rl)) {
			unsigned long now = get_jiffies();
			unsigned long ms, df, di, fps10;

			if (!mark_j) {
				mark_j = now;
				mark_f = vrx_frames;
				mark_i = sched_idle_us();
				continue;
			}

			ms = now - mark_j;
			df = vrx_frames - mark_f;
			di = sched_idle_us() - mark_i;
			if (!ms)
				ms = 1;
			fps10 = (df * 10000UL) / ms;

			printk("[VRX] %lu frames, %lu.%lu/s, %lu whole,"
			       " %lu%% idle\n",
			       vrx_frames, fps10 / 10UL, fps10 % 10UL,
			       vrx_complete, di / (ms * 10UL));

			if (ratelimit_allow(&vrx_probe_rl)) {
				uvc_dump_state();
				lcdc_dump_health();
			}

			if (vrx_bad || vrx_late || vrx_sock.rx_dropped)
				printk("[VRX] bad %lu, late %lu, no slot %lu\n",
				       vrx_bad, vrx_late, vrx_sock.rx_dropped);

			mark_j = now;
			mark_f = vrx_frames;
			mark_i = sched_idle_us();
		}
	}
}

static int __init video_rx_init(void)
{
	struct task_struct *t;

	if (udp_bind(&vrx_sock, VIDEO_PORT_RX, "video-rx") != 0)
		return -1;

	/*
	 * Above the sender, for the reason sched.h gives: an arriving frame
	 * that is not taken out of its slot is overwritten and gone, while one
	 * waiting to be sent is only late.
	 */
	t = task_create(video_rx_task, PRIO_VIDEO_RX, "video-rx");
	if (!t) {
		printk("[VRX] could not create the receiver task\n");
		return -1;
	}
	enqueue_task(&runqueue, t);

	return 0;
}
late_initcall(video_rx_init);

#endif /* CONFIG_VIDEO_STREAM */
