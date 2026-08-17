/*
 * net/udp_source.c - transmit as fast as the machine allows, and say how fast
 *
 * The receive direction has a number now: line rate, at the smallest packet
 * there is.  Transmit has only ever had an estimate, derived from how round
 * trip latency grew with datagram size, and this week has been a steady lesson
 * in how far those land from the measurement.
 *
 * Asking for the traffic rather than sending it unprompted is not a
 * limitation, it is what keeps the address question closed one step longer.
 * The request says where the answer goes, exactly as every protocol here has
 * so far, so nothing needs an ARP cache or an opinion about where this board's
 * peer lives.  Those arrive when a box has to start a conversation instead of
 * continuing one.
 *
 *   BLST <count:u32> <size:u16>   →   count datagrams of size bytes, numbered
 *
 * Port 19, where a character generator has always been.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/sched.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/printk.h>

#define UDP_SOURCE_PORT		19
#define REQUEST_LEN		10		/* "BLST" + u32 + u16 */
#define SOURCE_MAX		2000000UL	/* a bound, not a policy */

static struct udp_sock source_sock;

/*
 * The frame being built, in static storage.  A kernel task has one page of
 * stack and this is a third of it; more to the point, the same buffer is
 * refilled a million times and putting it on the stack would say otherwise.
 */
static u8 source_payload[UDP_MAX_PAYLOAD];

static void udp_source_task(void)
{
	for (;;) {
		struct udp_datagram *dg = udp_recv(&source_sock);
		u8 mac[ETH_ALEN], ip[IP_ALEN];
		unsigned long count, i, sent = 0, failed = 0;
		unsigned long start, ms;
		unsigned int size;
		u16 port;

		if (dg->len < REQUEST_LEN ||
		    dg->data[0] != 'B' || dg->data[1] != 'L' ||
		    dg->data[2] != 'S' || dg->data[3] != 'T') {
			udp_done(&source_sock);
			continue;
		}

		count = ((unsigned long)dg->data[4] << 24) |
			((unsigned long)dg->data[5] << 16) |
			((unsigned long)dg->data[6] << 8)  |
			 (unsigned long)dg->data[7];
		size  = ((unsigned int)dg->data[8] << 8) | dg->data[9];

		/*
		 * The requester's address is copied out, not held.  A run of a
		 * million frames cannot keep a receive slot occupied for its
		 * whole duration — that is one eighth of the socket's ring
		 * held against traffic still arriving.
		 */
		for (i = 0; i < ETH_ALEN; i++)
			mac[i] = dg->src_mac[i];
		for (i = 0; i < IP_ALEN; i++)
			ip[i] = dg->src_ip[i];
		port = dg->src_port;

		udp_done(&source_sock);

		if (size < 4)
			size = 4;		/* room for the sequence number */
		if (size > UDP_MAX_PAYLOAD)
			size = UDP_MAX_PAYLOAD;
		if (count > SOURCE_MAX)
			count = SOURCE_MAX;

		printk("[SRC] %lu datagrams of %u bytes to"
		       " %lu.%lu.%lu.%lu:%lu\n",
		       count, size,
		       (unsigned long)ip[0], (unsigned long)ip[1],
		       (unsigned long)ip[2], (unsigned long)ip[3],
		       (unsigned long)port);

		start = get_jiffies();

		for (i = 0; i < count; i++) {
			source_payload[0] = (u8)(i >> 24);
			source_payload[1] = (u8)(i >> 16);
			source_payload[2] = (u8)(i >> 8);
			source_payload[3] = (u8)i;

			if (udp_send(&source_sock, mac, ip, port,
				     source_payload, size) == 0)
				sent++;
			else
				failed++;
		}

		ms = get_jiffies() - start;
		if (!ms)
			ms = 1;

		/*
		 * On the wire a datagram costs 66 bytes more than its payload:
		 * UDP, IP and Ethernet headers, the CRC, the preamble and the
		 * inter-frame gap.  Reporting payload alone would understate
		 * what the link is actually doing by about a tenth at full
		 * size, and by two thirds at the smallest.
		 *
		 * Derived from the rate rather than the total, because the
		 * total does not fit: two million frames of 1538 bytes in bits
		 * is 2.4e10, and this is a 32-bit machine.  Dividing by 125
		 * rather than multiplying by 8 and dividing by 1000 keeps the
		 * one intermediate value small — a rate times a frame size is
		 * about 1.2e7 at line rate, whichever size it is.
		 */
		{
			unsigned long per_s = (sent * 1000UL) / ms;

			printk("[SRC] sent %lu in %lu ms = %lu/s,"
			       " %lu kbit/s on the wire, %lu failed\n",
			       sent, ms, per_s,
			       (per_s * (size + 66UL)) / 125UL,
			       failed);
		}
	}
}

static int __init udp_source_init(void)
{
	struct task_struct *t;

	if (udp_bind(&source_sock, UDP_SOURCE_PORT, "source") != 0)
		return -1;

	/*
	 * The transmit half of the VIDEO band, one level below the receiver —
	 * see the note in sched.h for why that order and not the other.
	 *
	 * The first attempt put both at PRIO_VIDEO and the kernel refused to
	 * boot, naming both tasks: deadline levels are exclusive, because two
	 * tasks sharing one is two tasks with the same deadline and no way to
	 * arbitrate between them.  A band four levels wide is the answer to
	 * that, and the panic is the reason this comment knows it.
	 *
	 * It sits above the UI, so the console is quiet for the length of a
	 * run — but cpsw_tx sleeps waiting for each frame rather than spinning,
	 * so this yields on every datagram and starves nothing.
	 */
	t = task_create(udp_source_task, PRIO_VIDEO_TX, "udp-source");
	if (!t) {
		printk("[SRC] could not create the source task\n");
		return -1;
	}
	enqueue_task(&runqueue, t);

	return 0;
}
late_initcall(udp_source_init);
