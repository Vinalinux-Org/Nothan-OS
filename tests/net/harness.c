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

static int host_tx(struct netdev *dev, const u8 *frame, unsigned int len)
{
	unsigned int i;

	(void)dev;
	printf("TX ");
	for (i = 0; i < len; i++)
		printf("%02x", frame[i]);
	printf("\n");
	fflush(stdout);
	return 0;
}

static struct netdev host_dev = {
	.name = "test0",
	.mac  = { 0x88, 0x0c, 0xe0, 0x50, 0x50, 0xd6 },
	.tx   = host_tx,
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
