/*
 * net/udp.c - datagrams in, datagrams answered
 *
 * The receive half runs in the driver's task and does as little as it can: it
 * checks the datagram, finds the socket, copies the payload into a free slot
 * and wakes the owner.  Everything a datagram is actually for happens in the
 * owner's band, which is the whole reason a socket is a ring and not a
 * callback — see the note at the top of nothan/udp.h.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/ipv4.h>
#include <nothan/udp.h>
#include <nothan/wait.h>
#include <nothan/printk.h>

/*
 * PROTECTION: none needed.  Sockets are bound during initcalls, before the
 * receive task exists, and the list is only read afterwards.
 */
static struct udp_sock *bound;

static unsigned long udp_short;		/* too small to hold a header */
static unsigned long udp_bad_len;	/* length field disagrees with the frame */
static unsigned long udp_bad_checksum;
static unsigned long udp_no_socket;

static inline u16 be16(const u8 *p)
{
	return (u16)(((u16)p[0] << 8) | p[1]);
}

static inline void put_be16(u8 *p, u16 v)
{
	p[0] = (u8)(v >> 8);
	p[1] = (u8)v;
}

/*
 * The pseudo header: the addresses and length UDP does not carry but does
 * protect.  It exists so a datagram delivered to the wrong host or with a
 * rewritten length fails the checksum instead of arriving looking correct.
 *
 * Returned as a running sum rather than a checksum, because ip_checksum()
 * takes a seed for exactly this: the pseudo header and the datagram are one
 * sum computed over two spans, and folding twice would be wrong.
 */
static u32 udp_pseudo_sum(const u8 *sip, const u8 *dip, unsigned int udp_len)
{
	u32 sum = 0;

	sum += ((u32)sip[0] << 8) | sip[1];
	sum += ((u32)sip[2] << 8) | sip[3];
	sum += ((u32)dip[0] << 8) | dip[1];
	sum += ((u32)dip[2] << 8) | dip[3];
	sum += IPPROTO_UDP;
	sum += udp_len;

	return sum;
}

int udp_bind(struct udp_sock *s, u16 port, const char *name)
{
	struct udp_sock *p;

	for (p = bound; p; p = p->next) {
		if (p->port == port) {
			printk("[UDP] port %lu refused for %s: %s has it\n",
			       (unsigned long)port, name, p->name);
			return -1;
		}
	}

	s->port = port;
	s->name = name;
	s->rx.head = 0;
	s->rx.tail = 0;
	init_waitqueue_head(&s->wq);

	s->next = bound;
	bound = s;

	printk("[UDP] %s bound to port %lu, %d slots of %lu bytes\n",
	       name, (unsigned long)port, 1 << UDP_RING_ORDER,
	       (unsigned long)UDP_MAX_PAYLOAD);
	return 0;
}

static struct udp_sock *udp_lookup(u16 port)
{
	struct udp_sock *s;

	for (s = bound; s; s = s->next) {
		if (s->port == port)
			return s;
	}
	return (struct udp_sock *)0;
}

void udp_input(struct netdev *dev, const u8 *frame,
	       const u8 *iphdr, unsigned int iphdr_len,
	       const u8 *payload, unsigned int payload_len)
{
	struct udp_sock *s;
	struct udp_datagram *dg;
	unsigned int udp_len, data_len, i;
	const u8 *data;

	(void)dev;
	(void)iphdr_len;

	if (payload_len < UDP_HDR_LEN) {
		udp_short++;
		return;
	}

	udp_len = be16(payload + UDP_LEN);

	/*
	 * The length field is checked against what actually arrived, in both
	 * directions.  Below the header size it is nonsense; above what the IP
	 * layer handed over it is an instruction to read past the frame.
	 */
	if (udp_len < UDP_HDR_LEN || udp_len > payload_len) {
		udp_bad_len++;
		return;
	}

	/*
	 * A zero checksum means the sender did not compute one, which IPv4
	 * permits.  Nothing else is optional: a checksum that is present and
	 * wrong is a corrupt datagram, not a hint.
	 */
	if (be16(payload + UDP_CHECKSUM) != 0) {
		u32 seed = udp_pseudo_sum(iphdr + IP_SRC, iphdr + IP_DST,
					  udp_len);

		if (ip_checksum(payload, udp_len, seed) != 0) {
			udp_bad_checksum++;
			return;
		}
	}

	s = udp_lookup(be16(payload + UDP_DST_PORT));
	if (!s) {
		/*
		 * No ICMP port unreachable.  Announcing which ports are closed
		 * is a service to whoever is scanning, and the box has nobody
		 * to tell: on this link, anything that matters knows the port
		 * because both ends were built together.
		 */
		udp_no_socket++;
		return;
	}

	data     = payload + UDP_HDR_LEN;
	data_len = udp_len - UDP_HDR_LEN;

	dg = udp_rx_reserve(&s->rx);
	if (!dg) {
		/*
		 * Every slot is still held by the owner.  Dropping the newest
		 * is what keeps this path lock free — taking the oldest would
		 * mean the producer moving the consumer's index.  An owner that
		 * would rather have fresh data than complete data can drain its
		 * own ring; see the note in ring.h.
		 */
		s->rx_dropped++;
		return;
	}

	for (i = 0; i < ETH_ALEN; i++)
		dg->src_mac[i] = frame[ETH_ALEN + i];
	for (i = 0; i < IP_ALEN; i++)
		dg->src_ip[i] = iphdr[IP_SRC + i];

	dg->src_port = be16(payload + UDP_SRC_PORT);
	dg->len      = data_len;

	for (i = 0; i < data_len; i++)
		dg->data[i] = data[i];

	udp_rx_commit(&s->rx);
	s->rx_datagrams++;

	wake_up(&s->wq);
}

struct udp_datagram *udp_recv(struct udp_sock *s)
{
	struct udp_datagram *dg;

	/*
	 * A loop inside wait_event_cond(), not an if: peek can still come back
	 * empty after a wakeup.  Here it cannot — one consumer, and it is this
	 * task — but the loop costs nothing and stops being merely true when a
	 * second reader is ever added.
	 */
	wait_event_cond(&s->wq, (dg = udp_rx_peek(&s->rx)) != (void *)0);

	return dg;
}

void udp_done(struct udp_sock *s)
{
	udp_rx_release(&s->rx);
}

int udp_reply(struct udp_sock *s, const struct udp_datagram *dg,
	      const u8 *data, unsigned int len)
{
	struct netdev *dev = netdev_get();
	unsigned int total, i;
	u8 *ip, *udp;
	u16 sum;

	if (!dev)
		return -1;

	if (len > UDP_MAX_PAYLOAD)
		return -1;

	total = ETH_HDR_SIZE + IP_MIN_HDR + UDP_HDR_LEN + len;

	/* Ethernet: back where it came from, from us. */
	for (i = 0; i < ETH_ALEN; i++) {
		s->tx[i]            = dg->src_mac[i];
		s->tx[ETH_ALEN + i] = dev->mac[i];
	}
	put_be16(s->tx + 12, ETH_P_IPV4);

	ip  = s->tx + ETH_HDR_SIZE;
	udp = ip + IP_MIN_HDR;

	ip[IP_VER_IHL] = 0x45;		/* version 4, no options */
	ip[IP_TOS]     = 0;
	put_be16(ip + IP_TOT_LEN, (u16)(IP_MIN_HDR + UDP_HDR_LEN + len));

	/*
	 * Identification zero.  It exists to group fragments, and a datagram
	 * that is never fragmented has no group — RFC 6864 says a receiver may
	 * not read it for anything else.  Inventing a counter here would be a
	 * number with no reader.
	 */
	put_be16(ip + IP_ID, 0);
	put_be16(ip + IP_FRAG, 0);

	ip[IP_TTL]   = 64;
	ip[IP_PROTO] = IPPROTO_UDP;

	for (i = 0; i < IP_ALEN; i++) {
		ip[IP_SRC + i] = ipv4_addr()[i];
		ip[IP_DST + i] = dg->src_ip[i];
	}

	put_be16(ip + IP_CHECKSUM, 0);
	put_be16(ip + IP_CHECKSUM, ip_checksum(ip, IP_MIN_HDR, 0));

	put_be16(udp + UDP_SRC_PORT, s->port);
	put_be16(udp + UDP_DST_PORT, dg->src_port);
	put_be16(udp + UDP_LEN, (u16)(UDP_HDR_LEN + len));
	put_be16(udp + UDP_CHECKSUM, 0);

	for (i = 0; i < len; i++)
		udp[UDP_HDR_LEN + i] = data[i];

	sum = ip_checksum(udp, UDP_HDR_LEN + len,
			  udp_pseudo_sum(ip + IP_SRC, ip + IP_DST,
					 UDP_HDR_LEN + len));

	/*
	 * A computed zero goes on the wire as all ones.  The two are the same
	 * number in one's complement, and zero is spoken for: it is how a
	 * sender says it computed nothing.  Sending the zero would turn a
	 * checked datagram into an unchecked one at random.
	 */
	put_be16(udp + UDP_CHECKSUM, sum ? sum : 0xFFFFu);

	if (dev->tx(dev, s->tx, total) != 0)
		return -1;

	netdev_stats.tx_frames++;
	s->tx_datagrams++;
	return 0;
}

void udp_dump_stats(void)
{
	struct udp_sock *s;

	printk("[UDP] %lu short, %lu bad length, %lu bad checksum,"
	       " %lu no socket\n",
	       udp_short, udp_bad_len, udp_bad_checksum, udp_no_socket);

	for (s = bound; s; s = s->next)
		printk("[UDP]   :%lu %s — %lu in, %lu out, %lu dropped\n",
		       (unsigned long)s->port, s->name,
		       s->rx_datagrams, s->tx_datagrams, s->rx_dropped);
}
