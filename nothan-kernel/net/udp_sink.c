/*
 * net/udp_sink.c - receive in one direction and say how fast, so the numbers
 * come from the shape of the real workload
 *
 * Every measurement so far has been an echo: one datagram in, one out, and a
 * sender that waits.  A box receiving video does none of those things.  It
 * takes a stream that does not pause for it, does not answer, and cannot ask
 * for anything to be sent again — so the question "how much can arrive before
 * something is lost" has never actually been asked, and the eight slots a
 * socket has were chosen with nothing to choose from.
 *
 * This asks it.  Bind a port, drain it as fast as the machine allows, and
 * report the rate and where the losses were.  Draining without doing any work
 * is deliberate: the answer wanted is the ceiling of the plumbing, before a
 * decoder takes its share.  A real consumer will be slower and this says by
 * how much it may be.
 *
 * The sequence number is what separates two kinds of loss that look identical
 * from the far end.  Gaps count everything that never reached this task;
 * rx_dropped counts what reached the socket and found no free slot.  The
 * difference is what was lost before that — in the driver, the MAC, or the
 * wire — and knowing which of the two is larger is the difference between
 * making the ring deeper and making the reader faster.
 *
 * Port 9, where a discard service has always been.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/sched.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/printk.h>

#define UDP_SINK_PORT		9

static struct udp_sock sink_sock;

/* One line a second: enough to watch a rate move, few enough to read. */
DEFINE_RATELIMIT(sink_rl, 1000, 1);

static unsigned long sink_datagrams;
static unsigned long sink_bytes;
static unsigned long sink_gaps;		/* never arrived, from any cause */
static unsigned long sink_reordered;	/* arrived after something newer */
static unsigned long sink_runts;	/* too short to carry a sequence */

static void udp_sink_task(void)
{
	unsigned long expect = 0;
	unsigned long mark_j = 0, mark_n = 0, mark_b = 0;
	int running = 0;

	for (;;) {
		struct udp_datagram *dg = udp_recv(&sink_sock);
		unsigned long seq;

		if (dg->len < 4) {
			sink_runts++;
			udp_done(&sink_sock);
			continue;
		}

		seq = ((unsigned long)dg->data[0] << 24) |
		      ((unsigned long)dg->data[1] << 16) |
		      ((unsigned long)dg->data[2] << 8)  |
		       (unsigned long)dg->data[3];

		sink_datagrams++;
		sink_bytes += dg->len;

		/*
		 * A run starts at whatever the first sequence happens to be,
		 * so a sender that has been going a while does not read as one
		 * enormous gap.  Zero restarts it, which is how each run of the
		 * blaster announces itself.
		 */
		if (!running || seq == 0) {
			running = 1;
			expect  = seq;
			mark_j  = get_jiffies();
			mark_n  = sink_datagrams;
			mark_b  = sink_bytes;
		}

		if (seq > expect)
			sink_gaps += seq - expect;
		else if (seq < expect)
			sink_reordered++;

		expect = seq + 1;

		udp_done(&sink_sock);

		if (ratelimit_allow(&sink_rl)) {
			unsigned long now = get_jiffies();
			unsigned long ms  = now - mark_j;
			unsigned long dn  = sink_datagrams - mark_n;
			unsigned long db  = sink_bytes - mark_b;

			if (!ms)
				ms = 1;

			/*
			 * Bits per millisecond is kilobits per second, which
			 * keeps the whole calculation inside 32 bits at any
			 * rate this link can reach.  Multiplying bytes by the
			 * tick rate first would not.
			 */
			printk("[SINK] %lu datagrams, %lu kbit/s, %lu/s;"
			       " %lu gaps (%lu no slot), %lu reordered\n",
			       sink_datagrams,
			       (db * 8UL) / ms,
			       (dn * 1000UL) / ms,
			       sink_gaps, sink_sock.rx_dropped,
			       sink_reordered);

			mark_j = now;
			mark_n = sink_datagrams;
			mark_b = sink_bytes;
		}
	}
}

static int __init udp_sink_init(void)
{
	struct task_struct *t;

	if (udp_bind(&sink_sock, UDP_SINK_PORT, "sink") != 0)
		return -1;

	/*
	 * The receive half of the VIDEO band, because that is where the reader
	 * of a stream like this one will actually run.  Measuring it in the
	 * driver's own band would answer a question nobody is going to ask.
	 */
	t = task_create(udp_sink_task, PRIO_VIDEO_RX, "udp-sink");
	if (!t) {
		printk("[SINK] could not create the sink task\n");
		return -1;
	}
	enqueue_task(&runqueue, t);

	return 0;
}
late_initcall(udp_sink_init);
