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
	 * There was a 1536-byte send buffer here, one per socket, because two
	 * owner tasks building replies through one shared buffer would
	 * interleave two datagrams into one frame.  The link now hands out the
	 * buffer it will send from, and hands out one at a time, so the
	 * exclusion that made a per-socket copy necessary is the same exclusion
	 * that makes it redundant.
	 */
	unsigned long		rx_datagrams;
	unsigned long		rx_dropped;	/* no free slot */
	unsigned long		tx_datagrams;
};

/*
 * Bind a socket to a port.  Initcall time only — see the note on @next.
 * Returns 0, or -1 if the port is already taken.
 *
 * A port of 0 registers the socket without an address, for a pool whose
 * entries are handed out later by udp_port_claim().  It is what lets a socket
 * appear at runtime without the list itself ever changing.
 */
int udp_bind(struct udp_sock *s, u16 port, const char *name);

/*
 * Give an already-registered socket an address, or take it away again.
 *
 * These exist so that a user process can open and close sockets without
 * anything being added to or removed from the list @next threads.  Removal is
 * the operation that would race: a receive task walking the list while a link
 * is unhooked from under it has no safe outcome, and the alternative is a lock
 * on the hottest path in the network stack.  Changing a port cannot break a
 * walk, so the pool is registered once at boot and its entries are lent out.
 *
 * udp_port_claim() returns 0, or -1 if the port is 0 or already taken.
 */
int  udp_port_claim(struct udp_sock *s, u16 port);
void udp_port_release(struct udp_sock *s);

/*
 * Borrow the oldest datagram, sleeping until one arrives.  The pointer stays
 * valid until udp_done(); the slot cannot be refilled before then.
 */
struct udp_datagram *udp_recv(struct udp_sock *s);

/*
 * The same borrow, without the sleep: the oldest datagram, or NULL if none has
 * arrived.  Released by udp_done() exactly as udp_recv()'s is.
 *
 * For a task that has other work to get on with — a sender pushing out frames
 * that must still notice a control message between them.  Blocking there would
 * stop the stream to wait for an instruction that will probably never come.
 */
struct udp_datagram *udp_poll(struct udp_sock *s);

/* Give the slot back.  Nothing may read through the pointer afterwards. */
void udp_done(struct udp_sock *s);

/*
 * Send to an address the caller already knows, link address included.
 *
 * Naming the three destinations separately rather than taking a datagram is
 * what lets a sender outlive the datagram that told it where to send: a task
 * blasting a thousand frames cannot hold a receive slot for the duration, and
 * a copy of one on the stack is 1512 bytes against a 4 KB kernel stack.
 *
 * It is also the shape the ARP cache slots into: udp_send_to() below is this
 * function with @dst_mac come from a lookup instead of an argument, and this
 * signature did not have to change to gain it.
 */
int udp_send(struct udp_sock *s, const u8 *dst_mac, const u8 *dst_ip,
	     u16 dst_port, const u8 *data, unsigned int len);

/**
 * udp_send_to() - send to an IP address, resolving the link address
 *
 * For a sender that was not handed an address by an incoming frame — the one
 * that speaks first.  Everything that came before could use udp_reply().
 *
 * Fails while the address is being resolved, and does not sleep waiting for
 * it: see arp_resolve().  A first send to a new peer will therefore usually
 * fail, and the retry a moment later is the one that goes out.  That is a
 * property of the caller's protocol, not a bug to paper over here — chat
 * retransmits anyway, so it costs nothing, whereas hiding the wait inside this
 * call would mean sleeping while holding the link's only transmit buffer.
 *
 * Return: 0 on success, -1 if the address is not yet known or the send failed.
 */
int udp_send_to(struct udp_sock *s, const u8 *dst_ip, u16 dst_port,
		const u8 *data, unsigned int len);

/* Answer a borrowed datagram: the frame that arrived says where to reply. */
static inline int udp_reply(struct udp_sock *s, const struct udp_datagram *dg,
			    const u8 *data, unsigned int len)
{
	return udp_send(s, dg->src_mac, dg->src_ip, dg->src_port, data, len);
}

/* Called by the IP layer for protocol 17. */
void udp_input(struct netdev *dev, const u8 *frame,
	       const u8 *iphdr, unsigned int iphdr_len,
	       const u8 *payload, unsigned int payload_len);

void udp_dump_stats(void);

#endif /* _NOTHAN_UDP_H */
