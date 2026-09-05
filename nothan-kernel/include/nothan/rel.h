#ifndef _NOTHAN_REL_H
#define _NOTHAN_REL_H

/*
 * include/nothan/rel.h - the reliable half of os-architecture.md §7.2
 *
 * The doc asks for two transports and gives the reason for each: datagrams for
 * audio and video, where a frame retransmitted after its deadline is worse
 * than a frame missing, and something ordered and lossless for chat and call
 * control, where a message that never lands is a conversation that quietly
 * stops making sense.  This is the second one.
 *
 * NOT TCP, and the doc is specific about which parts are being left out and
 * why: congestion control assumes a shared Internet, and this is a cable
 * between two machines; window scaling, SACK and Nagle are all answers to
 * bandwidth-delay products this link does not have; the connection teardown
 * states exist to resolve ambiguity about who hung up, which one peer and one
 * conversation do not have either.  What is actually needed is sequence,
 * acknowledgement and retransmission.
 *
 * THREE DIVERGENCES FROM THE DOC, each with a reason:
 *
 * 1. It rides inside UDP rather than beside it as its own IP protocol.  The
 *    diagram draws them as siblings, and as a picture of layering that is
 *    right, but a protocol number of its own means ports, checksums, socket
 *    demultiplexing and every test tool have to be built again.  Inside UDP,
 *    the whole of the existing socket path carries it unchanged and the far
 *    end is a dozen lines of ordinary Python.  QUIC made the same trade for
 *    the same reason.  Being able to test it is worth more here than being
 *    able to draw it flat.
 *
 * 2. Stop-and-wait: one message outstanding, not a sliding window.  A window
 *    is machinery for keeping a pipe full, and this pipe carries a person
 *    typing.  Nothing in chat or in call control produces the back-to-back
 *    traffic that would pay for it, and an unused window is code that is
 *    maintained but never exercised — which is to say, code that is wrong and
 *    nobody knows.
 *
 * 3. No handshake.  Each socket picks a session number when it opens, and a
 *    receiver seeing an unfamiliar one resets and believes it.  A three-way
 *    handshake exists to agree initial sequence numbers and to detect a peer
 *    that restarted; the session number detects the restart directly, which
 *    was the part that mattered, and on a private cable the part it guards
 *    against — an attacker guessing sequence numbers — has nobody to be.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/udp.h>
#include <nothan/ring.h>

/*
 * Wire format, 12 bytes, big endian like everything else on this wire.
 *
 *   0  magic    u16   refuses anything that is not this protocol
 *   2  type     u8    DATA or ACK
 *   3  reserved u8    must be zero; a field with a checker is a field that
 *                     can be given a meaning later without a flag day
 *   4  session  u32   identifies the stream, not the sender
 *   8  seq      u32   position in that stream
 *  12  payload
 *
 * An ACK carries the session and sequence of the DATA it answers, not its
 * own.  That is what lets a sender match an acknowledgement to the message it
 * is still holding without keeping a table of who is answering whom.
 */
#define REL_MAGIC		0x4E54u		/* 'N','T' */
#define REL_HDR_LEN		12

#define REL_OFF_MAGIC		0
#define REL_OFF_TYPE		2
#define REL_OFF_RSVD		3
#define REL_OFF_SESSION		4
#define REL_OFF_SEQ		8

#define REL_TYPE_DATA		1
#define REL_TYPE_ACK		2

/*
 * One message.  512 bytes is a chat line with room to be wrong about how long
 * a chat line is, and it keeps a socket's two rings inside 8 KB — the ceiling
 * that matters, since four sockets share a kernel with 512 MB and no swap.
 */
#define REL_MSG_MAX		512

struct rel_msg {
	u8		peer_ip[IP_ALEN];
	u16		peer_port;
	u16		len;
	u8		data[REL_MSG_MAX];
};

/*
 * Four waiting to go out, eight waiting to be read.
 *
 * Asymmetric on purpose.  The send queue is drained by this box at the speed
 * of the wire and filled by a person typing, so it is never deep; the receive
 * queue is filled by the wire and drained by an application that might be
 * three frames into a redraw, so it is the one that needs slack.
 */
#define REL_TXQ_ORDER		2
#define REL_RXQ_ORDER		3

DEFINE_RING(rel_tx, struct rel_msg, REL_TXQ_ORDER);
DEFINE_RING(rel_rx, struct rel_msg, REL_RXQ_ORDER);

/*
 * 200 ms between attempts, ten attempts.
 *
 * A reply on this link comes back in well under a millisecond, so 200 ms is
 * not a round-trip estimate — it is how long to wait before deciding that
 * silence means loss rather than slowness, and it is chosen to be far longer
 * than any real answer so that a retransmission is evidence of a real problem
 * rather than of impatience.  Ten of them is two seconds, which is about as
 * long as a person will believe a message is still sending.
 */
#define REL_RETRY_MS		200
#define REL_RETRIES		10

/*
 * How often the transport wakes to look at its sockets.
 *
 * Polling rather than sleeping on the socket's wait queue, because this task
 * has two things to wait for and only one of them is a datagram: a
 * retransmission is due at a time, not on an event, and a task blocked in
 * udp_recv() cannot notice a deadline pass.  20 ms costs fifty wake-ups a
 * second on a box that is 94% idle, and buys one code path instead of two.
 */
#define REL_TICK_MS		20

struct rel_sock {
	struct udp_sock	*udp;		/* the datagram socket underneath */
	int		open;

	/* Sending. */
	u32		session;	/* ours, fixed for the life of the socket */
	u32		next_seq;
	int		in_flight;	/* the head of @txq has been sent */
	u32		flight_seq;
	unsigned long	deadline;	/* jiffies; when to send it again */
	int		retries;

	/*
	 * Receiving.  @peer_seq is the last sequence delivered upward, and is
	 * only meaningful while @peer_session matches what is arriving.
	 */
	int		peer_known;
	u32		peer_session;
	u32		peer_seq;

	struct rel_tx_ring	txq;
	struct rel_rx_ring	rxq;

	unsigned long	tx_msgs;
	unsigned long	retransmits;
	unsigned long	gave_up;
	unsigned long	rx_msgs;
	unsigned long	rx_dups;
	unsigned long	rx_gaps;
	unsigned long	rx_malformed;
};

/*
 * Attach reliability to an already-bound datagram socket, or take it away.
 *
 * The socket underneath keeps working exactly as it did; what changes is who
 * drains it.  From rel_attach() until rel_detach(), the transport task is the
 * only consumer of that socket's ring — ring.h allows exactly one — so nothing
 * else may call udp_recv() or udp_poll() on it.
 */
void rel_attach(struct rel_sock *r, struct udp_sock *udp);
void rel_detach(struct rel_sock *r);

/*
 * Queue a message.  Returns 0, or -1 if the send queue is full.
 *
 * Does not wait for the acknowledgement, and does not fail because the peer's
 * link address is not resolved yet: the retransmission machinery covers ARP
 * for free, since a first attempt that cannot be addressed is the same event
 * as a first attempt that was lost.
 */
int rel_send(struct rel_sock *r, const u8 *ip, u16 port,
	     const void *data, unsigned int len);

/*
 * Take the oldest delivered message, or return 0 if none.  Never blocks.
 * @out receives the sender's address; @buf gets up to @len bytes.
 */
int rel_recv(struct rel_sock *r, u8 *out_ip, u16 *out_port,
	     void *buf, unsigned int len);

void rel_dump_stats(void);

#endif /* _NOTHAN_REL_H */
