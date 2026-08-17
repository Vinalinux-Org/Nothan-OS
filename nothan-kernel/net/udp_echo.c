/*
 * net/udp_echo.c - a datagram sent back, from a different band
 *
 * The same role ICMP echo played one layer down: something a person can check
 * from outside with no software written for the occasion.  A line of netcat
 * either comes back or it does not.
 *
 *   echo hello | nc -u -w1 10.42.0.2 7
 *
 * But it is checking a second thing that ping could not.  The datagram is
 * received by the driver's task in the NET band and answered by this task in
 * the BG band, so a reply arriving at all proves the hand-off across the ring
 * works: the slot survived, the owner woke, and the bytes were still there.
 * Comparing this round trip against ping's is what the band change costs,
 * measured rather than assumed.
 *
 * Port 7 because that is where an echo service has always been, so nothing has
 * to be told about it.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/sched.h>
#include <nothan/init.h>
#include <nothan/printk.h>

#define UDP_ECHO_PORT		7

static struct udp_sock echo_sock;

/* Same reasoning as ICMP: see nothan/printk.h. */
DEFINE_RATELIMIT(echo_rl, 1000, 1);

static void udp_echo_task(void)
{
	unsigned long seen = 0;

	for (;;) {
		struct udp_datagram *dg = udp_recv(&echo_sock);

		seen++;
		if (seen <= 4 || ratelimit_allow(&echo_rl))
			printk("[ECHO] %lu: %u bytes from"
			       " %lu.%lu.%lu.%lu:%lu\n",
			       seen, dg->len,
			       (unsigned long)dg->src_ip[0],
			       (unsigned long)dg->src_ip[1],
			       (unsigned long)dg->src_ip[2],
			       (unsigned long)dg->src_ip[3],
			       (unsigned long)dg->src_port);

		udp_reply(&echo_sock, dg, dg->data, dg->len);

		/*
		 * Released after the reply, not before.  The reply is built
		 * from the bytes in the slot, and giving the slot back first
		 * would let the receive task refill it while it is being read
		 * — a corruption that would only appear under load, which is
		 * the worst time to find it.
		 */
		udp_done(&echo_sock);
	}
}

static int __init udp_echo_init(void)
{
	struct task_struct *t;

	if (udp_bind(&echo_sock, UDP_ECHO_PORT, "echo") != 0)
		return -1;

	t = task_create(udp_echo_task, PRIO_BG, "udp-echo");
	if (!t) {
		printk("[ECHO] could not create the echo task\n");
		return -1;
	}
	enqueue_task(&runqueue, t);

	return 0;
}
late_initcall(udp_echo_init);
