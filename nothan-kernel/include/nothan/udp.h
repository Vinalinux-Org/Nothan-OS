#ifndef _NOTHAN_UDP_H
#define _NOTHAN_UDP_H

/*
 * include/nothan/udp.h - datagrams, and the band they are handled in
 *
 * os-architecture.md §7.2 wants datagrams for audio and video and something
 * reliable for chat.  This is the first half, and the shape of it is decided
 * by the scheduler rather than by the protocol: the bands exist to keep audio
 * (0) apart from video (8) apart from the UI (12), and a receive path that
 * runs every protocol in the driver's own band (NET, 4) throws that away at
 * exactly the point it starts to matter.
 *
 * So a socket is a ring of slots and a task that owns it.  The driver's
 * receive task copies a datagram into the next free slot and wakes the owner;
 * the owner runs in its own band and reads the bytes where they lie.  One copy
 * total, which is what it costs today anyway, and the band separation comes
 * free with it.
 *
 * Reading in place rather than copying out is not only about the copy.  It is
 * what makes the slot a fixed destination in DDR, which is where EDMA will
 * later be told to write.  When that happens the last copy disappears and this
 * header does not change: the ring stops caring who filled the slot.
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/ipv4.h>
#include <nothan/ring.h>
#include <nothan/wait.h>

/* Offsets from the start of the UDP header. */
#define UDP_SRC_PORT		0
#define UDP_DST_PORT		2
#define UDP_LEN			4
#define UDP_CHECKSUM		6
#define UDP_HDR_LEN		8

/*
 * The largest datagram that arrives in one frame.  Fragments are dropped by
 * the IP layer, so this is not a policy choice here — it is the whole of what
 * can ever be delivered.
 */
#define UDP_MAX_PAYLOAD		(ETH_FRAME_MAX - ETH_HDR_SIZE - IP_MIN_HDR \
				 - UDP_HDR_LEN)

/**
 * struct udp_datagram - one received datagram, and where to answer it
 *
 * The link address is kept alongside the protocol addresses because a reply
 * can then be sent without looking anything up.  Every protocol in this kernel
 * so far has only ever answered, and answering needs no table: the frame that
 * arrived already says where the answer goes.  That is why there is still no
 * ARP cache, and it stops being true the moment a box starts a conversation
 * instead of continuing one.
 */
struct udp_datagram {
	u8		src_mac[ETH_ALEN];
	u8		src_ip[IP_ALEN];
	u16		src_port;
	unsigned int	len;
	u8		data[UDP_MAX_PAYLOAD];
};

/*
 * Eight slots, MTU sized, the same for every socket.
 *
 * Not because every socket wants the same thing — audio will want many small
 * ones and video few large ones — but because nothing has measured that yet.
 * Sizing per socket is a knob that would have to be maintained, and chosen
 * from, and got wrong, before anything exists to tune it against.  Twelve
 * kilobytes per socket out of 512 MB is not a reason to build the knob.
 *
 * What would change this: a drop counter that moves under real audio.  At that
 * point the number has a source, and until then it would only have an author.
 */
#define UDP_RING_ORDER		3

DEFINE_RING(udp_rx, struct udp_datagram, UDP_RING_ORDER);

/**
 * struct udp_sock - a port, its slots, and the task waiting on them
 * @port:      host byte order, the port this socket answers on
 * @rx:        slots the receive task fills and the owner drains
 * @wq:        the owner sleeps here; the receive task wakes it
 * @next:      bound sockets, walked once per arriving datagram
 *
 * PROTECTION: none, and the reason is structural rather than lucky.  @rx has
 * one producer (the driver's receive task) and one consumer (the owner), which
 * is exactly ring.h's contract.  @next is written only by udp_bind() during
 * initcalls, before any frame can arrive.
 */
struct udp_sock {
	u16			port;
	const char		*name;
	struct udp_rx_ring	rx;
	struct wait_queue_head	wq;
	struct udp_sock		*next;

	/*
	 * Scratch for udp_reply(), per socket rather than one shared buffer.
	 * netdev tx serialises its callers, but the frame handed to it does
	 * not: two owner tasks replying at once through one static buffer
	 * would interleave two datagrams into one frame.  A socket has a
	 * single owner by the same contract that makes @rx lock free, so
	 * putting the buffer here inherits that and needs no lock.
	 */
	u8			tx[ETH_FRAME_MAX];

	unsigned long		rx_datagrams;
	unsigned long		rx_dropped;	/* no free slot */
	unsigned long		tx_datagrams;
};

/*
 * Bind a socket to a port.  Initcall time only — see the note on @next.
 * Returns 0, or -1 if the port is already taken.
 */
int udp_bind(struct udp_sock *s, u16 port, const char *name);

/*
 * Borrow the oldest datagram, sleeping until one arrives.  The pointer stays
 * valid until udp_done(); the slot cannot be refilled before then.
 */
struct udp_datagram *udp_recv(struct udp_sock *s);

/* Give the slot back.  Nothing may read through the pointer afterwards. */
void udp_done(struct udp_sock *s);

/*
 * Answer a borrowed datagram.  Destination address, port and link address all
 * come from @dg, so this needs no route and no ARP cache.  Returns 0 on
 * success.
 *
 * There is deliberately no udp_send_to() yet.  Sending to an address nobody
 * told us about needs a way to turn an IP into a MAC, and that is the ARP
 * cache — worth writing when something needs it, and not before.
 */
int udp_reply(struct udp_sock *s, const struct udp_datagram *dg,
	      const u8 *data, unsigned int len);

/* Called by the IP layer for protocol 17. */
void udp_input(struct netdev *dev, const u8 *frame,
	       const u8 *iphdr, unsigned int iphdr_len,
	       const u8 *payload, unsigned int payload_len);

void udp_dump_stats(void);

#endif /* _NOTHAN_UDP_H */
