/*
 * net/video_tx.c - take frames from the camera and put them on the wire
 *
 * The sending half of a call.  It knows nothing about where frames come from
 * beyond nothan/video.h, which is what lets it be written and measured while
 * the camera is still a generated pattern: when the USB webcam driver
 * registers, this file does not change.
 *
 * Streaming starts when someone asks for it, and where the answer goes is the
 * asker's own address.  That keeps this box out of the business of knowing
 * where its peer lives — still no ARP cache, still nothing that starts a
 * conversation rather than continuing one — and it is not a placeholder for
 * signalling either.  Whatever signalling a call eventually grows, it ends
 * with an address arriving over the network, which is this.
 *
 * A dropped datagram is not retried and a slow frame is not waited for.  If
 * transmit refuses, the chunk is counted and the loop moves to the next one:
 * the frame arrives at the far end with a hole in it, and one frame later the
 * hole is gone.  Stopping to recover a chunk would trade a defect lasting 33
 * milliseconds for a stall lasting longer.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/video.h>
#include <nothan/video_net.h>
#include <nothan/sched.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/delay.h>
#include <nothan/printk.h>

#if CONFIG_VIDEO_STREAM

static struct udp_sock video_sock;

/* One line a second while streaming: enough to watch a rate, few enough to read. */
DEFINE_RATELIMIT(vtx_rl, 1000, 1);

/* Where the stream goes, learned from whoever subscribed. */
static u8  peer_mac[ETH_ALEN];
static u8  peer_ip[IP_ALEN];
static u16 peer_port;
static int streaming;

static unsigned long vtx_frames;
static unsigned long vtx_chunks;
static unsigned long vtx_failed;	/* chunks the link refused */

/* The datagram being built: header then pixels, copied in by udp_send(). */
static u8 vtx_chunk[VIDEO_HDR_LEN + VIDEO_CHUNK];

static void put_be16(u8 *p, u16 v)
{
	p[0] = (u8)(v >> 8);
	p[1] = (u8)v;
}

static void put_be32(u8 *p, u32 v)
{
	p[0] = (u8)(v >> 24);
	p[1] = (u8)(v >> 16);
	p[2] = (u8)(v >> 8);
	p[3] = (u8)v;
}

static int tag_is(const struct udp_datagram *dg, const char *tag)
{
	return dg->len >= VIDEO_MSG_LEN &&
	       dg->data[0] == (u8)tag[0] && dg->data[1] == (u8)tag[1] &&
	       dg->data[2] == (u8)tag[2] && dg->data[3] == (u8)tag[3];
}

static void vtx_control(const struct udp_datagram *dg)
{
	unsigned int i;

	if (tag_is(dg, VIDEO_MSG_STOP)) {
		if (streaming)
			printk("[VTX] stopped after %lu frames\n", vtx_frames);
		streaming = 0;
		return;
	}

	if (!tag_is(dg, VIDEO_MSG_SUBSCRIBE))
		return;

	for (i = 0; i < ETH_ALEN; i++)
		peer_mac[i] = dg->src_mac[i];
	for (i = 0; i < IP_ALEN; i++)
		peer_ip[i] = dg->src_ip[i];
	peer_port = dg->src_port;
	streaming = 1;

	printk("[VTX] streaming to %lu.%lu.%lu.%lu:%lu\n",
	       (unsigned long)peer_ip[0], (unsigned long)peer_ip[1],
	       (unsigned long)peer_ip[2], (unsigned long)peer_ip[3],
	       (unsigned long)peer_port);
}

/*
 * One frame, in as many datagrams as it takes.
 *
 * The offset is what makes this a stream of independent pieces rather than a
 * sequence: nothing here depends on the previous datagram arriving, so a
 * receiver can drop any of them and still place the rest correctly.
 */
static void vtx_send_frame(const struct video_frame *f, unsigned int bytes)
{
	unsigned int off;

	for (off = 0; off < bytes; off += VIDEO_CHUNK) {
		unsigned int n = bytes - off;
		u8 flags = 0;

		if (n > VIDEO_CHUNK)
			n = VIDEO_CHUNK;
		else
			flags = VIDEO_F_LAST;

		put_be16(vtx_chunk + VIDEO_H_MAGIC, VIDEO_MAGIC);
		vtx_chunk[VIDEO_H_VERSION] = VIDEO_VERSION;
		vtx_chunk[VIDEO_H_FLAGS]   = flags;
		put_be32(vtx_chunk + VIDEO_H_SEQ, f->seq);
		put_be32(vtx_chunk + VIDEO_H_OFFSET, off);
		put_be16(vtx_chunk + VIDEO_H_WIDTH, f->width);
		put_be16(vtx_chunk + VIDEO_H_HEIGHT, f->height);

		/*
		 * Copied here and copied again by udp_send() into the link's
		 * buffer.  The second copy is nearly free — the checksum has to
		 * read every one of these bytes anyway, so the store rides
		 * along with a load that was already happening.  The first is
		 * not, and removing it needs a send that takes a header and a
		 * body separately.  Worth doing when 4.9% of the CPU for the
		 * sending half of a call is worth arguing about.
		 */
		{
			const u8 *src = f->pixels + off;
			unsigned int i;

			for (i = 0; i < n; i++)
				vtx_chunk[VIDEO_HDR_LEN + i] = src[i];
		}

		if (udp_send(&video_sock, peer_mac, peer_ip, peer_port,
			     vtx_chunk, VIDEO_HDR_LEN + n) == 0)
			vtx_chunks++;
		else
			vtx_failed++;
	}

	vtx_frames++;
}

static void video_tx_task(void)
{
	struct video_source *src;
	unsigned int bytes, dgrams;
	unsigned long wire;
	unsigned long mark_j = 0, mark_f = 0, mark_i = 0;

	/*
	 * Wait for a camera rather than deciding at startup that there is none.
	 *
	 * A generated source registers during initcalls, before this task first
	 * runs, and checking once was right while that was the only kind.  A
	 * USB camera registers when it has been plugged in, enumerated and
	 * negotiated with — seconds later, and possibly not until someone
	 * plugs it in tomorrow.  Giving up at boot would mean a box that has to
	 * be restarted after connecting its camera.
	 */
	while ((src = video_source_get()) == (void *)0)
		msleep(200);

	bytes  = video_frame_bytes(src);
	dgrams = (bytes + VIDEO_CHUNK - 1) / VIDEO_CHUNK;

	/*
	 * What a frame actually costs the link: its pixels, plus this
	 * protocol's header and the 66 bytes of UDP, IP, Ethernet, CRC,
	 * preamble and inter-frame gap that every datagram carries.  At this
	 * frame size that overhead is 5.6% — small, but reporting payload as
	 * though it were wire traffic is how a link gets called half loaded
	 * when it is nearly full.
	 */
	wire = bytes + (unsigned long)dgrams * (VIDEO_HDR_LEN + 66UL);

	printk("[VTX] %s, %lux%lu, %lu bytes in %lu datagrams a frame\n",
	       src->name,
	       (unsigned long)src->width, (unsigned long)src->height,
	       (unsigned long)bytes, (unsigned long)dgrams);

	for (;;) {
		struct udp_datagram *dg;
		struct video_frame *f;

		/*
		 * Control first, and the same call asks two different
		 * questions: idle, it blocks until somebody subscribes;
		 * streaming, it only glances and moves on.  Blocking while
		 * streaming would stop the stream to wait for an instruction
		 * that will probably never come.
		 */
		while ((dg = streaming ? udp_poll(&video_sock)
				       : udp_recv(&video_sock)) != (void *)0) {
			vtx_control(dg);
			udp_done(&video_sock);
		}

		if (!streaming)
			continue;

		f = src->capture(src);
		vtx_send_frame(f, bytes);
		src->release(src);

		if (ratelimit_allow(&vtx_rl)) {
			unsigned long now = get_jiffies();
			unsigned long ms, df, di;

			/*
			 * The first report has nothing to compare against, and
			 * printing it anyway said "1104% idle" — the whole
			 * uptime's idle time divided by one frame's worth of
			 * milliseconds.  Seed the marks and say nothing; a
			 * missing first line costs a second, a nonsense one
			 * costs whatever is spent believing it.
			 */
			if (!mark_j) {
				mark_j = now;
				mark_f = vtx_frames;
				mark_i = sched_idle_us();
				continue;
			}

			ms = now - mark_j;
			df = vtx_frames - mark_f;
			di = sched_idle_us() - mark_i;

			if (!ms)
				ms = 1;

			/*
			 * The rate is derived once and everything else built
			 * from it, which keeps every intermediate small: a
			 * frame count times a frame size in bits overflows
			 * thirty-two of them inside a minute, and dividing by
			 * 125 rather than multiplying by 8 and dividing by
			 * 1000 avoids the one place it would.
			 */
			unsigned long fps10 = (df * 10000UL) / ms;

			printk("[VTX] %lu frames, %lu.%lu/s, %lu kbit/s,"
			       " %lu%% idle\n",
			       vtx_frames,
			       fps10 / 10UL, fps10 % 10UL,
			       (fps10 * wire) / 1250UL,
			       di / (ms * 10UL));

			if (vtx_failed)
				printk("[VTX] %lu chunks refused by the link\n",
				       vtx_failed);

			mark_j = now;
			mark_f = vtx_frames;
			mark_i = sched_idle_us();
		}
	}
}

static int __init video_tx_init(void)
{
	struct task_struct *t;

	if (udp_bind(&video_sock, VIDEO_PORT, "video") != 0)
		return -1;

	t = task_create(video_tx_task, PRIO_VIDEO_TX, "video-tx");
	if (!t) {
		printk("[VTX] could not create the sender task\n");
		return -1;
	}
	enqueue_task(&runqueue, t);

	return 0;
}
late_initcall(video_tx_init);

#endif /* CONFIG_VIDEO_STREAM */
