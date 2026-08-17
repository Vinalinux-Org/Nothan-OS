/*
 * tests/net/harness.c - the protocol stack, on the host, with a real arbiter
 *
 * The network files are ordinary C with no hardware in them, so they can be
 * compiled for the machine the editor is running on and fed frames.  What that
 * buys is the difference between finding a length check off by one in a second
 * and finding it after writing a card, moving it, booting a board and reading
 * a log.
 *
 * It does not replace the board.  Nothing here exercises CPPI RAM, the memory
 * model, interrupt latency or the scheduler, and a pass proves only that the
 * protocol logic is right — which is most of what there is to get wrong above
 * the driver, and none of what there is to get wrong below it.
 *
 * Frames go in as hex on stdin, one per line.  Frames the stack decides to
 * send come back out as "TX <hex>", so the judging is done by a script that
 * knows what a correct answer looks like and did not write the stack.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/ipv4.h>
#include <nothan/udp.h>
#include <nothan/printk.h>

int printk(const char *fmt, ...)
{
	va_list ap;
	int n;

	/* Kernel chatter goes to stderr so stdout carries only frames. */
	va_start(ap, fmt);
	n = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return n;
}

/*
 * Time and rate limiting, stubbed to let everything through.  Suppressing log
 * lines on the host would only hide the stack's own commentary from stderr,
 * and the judge reads stdout.
 */
unsigned long get_jiffies(void) { return 0; }
int ratelimit_allow(struct ratelimit *r) { r->last_dropped = 0; return 1; }

/*
 * The link's transmit buffer, exactly as the driver holds one: the stack builds
 * its frame here and the "hardware" reads it back out.  @host_tx_held catches
 * the mistake this seam makes possible — building into a buffer that was never
 * claimed, or claiming one and never sending it.
 */
static u8  host_tx_buf[ETH_FRAME_MAX];
static int host_tx_held;

static u8 *host_tx_alloc(struct netdev *dev)
{
	(void)dev;

	if (host_tx_held) {
		fprintf(stderr, "harness: tx_alloc while already held\n");
		abort();
	}
	host_tx_held = 1;

	/*
	 * Poisoned, not cleared.  The buffer is now reused frame after frame,
	 * so a field the stack forgets to write no longer reads as zero — it
	 * reads as whatever the last frame left there, which is the kind of bug
	 * that works perfectly until the second packet.  Filling it with
	 * something that is obviously not a protocol makes the omission show up
	 * on the first frame instead of the second.
	 */
	memset(host_tx_buf, 0xAA, sizeof(host_tx_buf));
	return host_tx_buf;
}

static void host_tx_abort(struct netdev *dev)
{
	(void)dev;
	host_tx_held = 0;
}

static int host_tx_send(struct netdev *dev, unsigned int len)
{
	unsigned int i;

	(void)dev;

	if (!host_tx_held) {
		fprintf(stderr, "harness: tx_send without tx_alloc\n");
		abort();
	}

	/*
	 * No padding to the Ethernet minimum here, though the driver does it.
	 * That is the link's job and this harness is judging the stack — what
	 * gets printed is the frame the protocol layers built, to the length
	 * they declared.
	 */
	printf("TX ");
	for (i = 0; i < len; i++)
		printf("%02x", host_tx_buf[i]);
	printf("\n");
	fflush(stdout);

	host_tx_held = 0;
	return 0;
}

static struct netdev host_dev = {
	.name     = "test0",
	.mac      = { 0x88, 0x0c, 0xe0, 0x50, 0x50, 0xd6 },
	.tx_alloc = host_tx_alloc,
	.tx_send  = host_tx_send,
	.tx_abort = host_tx_abort,
};

/*
 * The echo service, inline rather than linked from net/udp_echo.c: that file
 * is a task and a scheduler, and the point here is the socket underneath it.
 * The behaviour copied is the part that matters — reply from the borrowed
 * slot, then release.
 */
static struct udp_sock echo;

static void drain_echo(void)
{
	struct udp_datagram *dg;

	while ((dg = udp_rx_peek(&echo.rx)) != NULL) {
		udp_reply(&echo, dg, dg->data, dg->len);
		udp_done(&echo);
	}
}

/* A socket nothing ever reads, to make the ring fill. */
static struct udp_sock deaf;

int main(void)
{
	char line[8192];

	netdev_register(&host_dev);
	udp_bind(&echo, 7, "echo");
	udp_bind(&deaf, 9, "deaf");

	while (fgets(line, sizeof(line), stdin)) {
		u8 frame[ETH_FRAME_MAX];
		unsigned int n = 0;
		char *p = line;

		while (*p && *(p + 1) && n < sizeof(frame)) {
			unsigned int b;

			if (sscanf(p, "%2x", &b) != 1)
				break;
			frame[n++] = (u8)b;
			p += 2;
		}

		if (!n)
			continue;

		netdev_rx(&host_dev, frame, n);
		drain_echo();

		/* A marker so the judge can tell "no reply" from "not yet". */
		printf("END\n");
		fflush(stdout);
	}

	printf("STATS deaf_in=%lu deaf_dropped=%lu deaf_wakes=%lu"
	       " echo_in=%lu echo_out=%lu echo_wakes=%lu\n",
	       deaf.rx_datagrams, deaf.rx_dropped, deaf.wq.wakes,
	       echo.rx_datagrams, echo.tx_datagrams, echo.wq.wakes);
	return 0;
}
