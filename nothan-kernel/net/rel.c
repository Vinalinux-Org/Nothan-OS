/*
 * net/rel.c - sequence, acknowledgement, retransmission
 *
 * The design and every divergence from os-architecture.md §7.2 are argued in
 * nothan/rel.h.  This file is the mechanism.
 *
 * ONE TASK FOR ALL SOCKETS.  A reliable socket has two things that must happen
 * whether or not the application is looking: an arriving message has to be
 * acknowledged, and an unacknowledged one has to be sent again.  Doing either
 * inside the application's own recv() would make both hostage to how busy the
 * application is — a GUI three frames into a redraw would stall the peer's
 * retransmit timer, and the peer would conclude the link had failed.  So the
 * transport runs on its own, and the application's socket calls only move
 * messages in and out of two rings.
 *
 * §7.2 makes the safety rule for this whole path: every arriving frame is
 * input nobody here wrote.  Check the length before reading a field, refuse
 * and count, never trust a header, and never panic on one — a malformed
 * datagram is an ordinary event on a wire, not a broken assumption.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/rel.h>
#include <nothan/udp.h>
#include <nothan/sched.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/time.h>
#include <nothan/delay.h>

#define REL_MAX_SOCKS	4

static struct rel_sock *rel_socks[REL_MAX_SOCKS];

/* Milliseconds as the scheduler counts them, from the one place HZ is set. */
#define REL_MS(ms)	((unsigned long)(ms) * HZ / 1000)

static int rel_due(unsigned long deadline)
{
	return (long)(get_jiffies() - deadline) >= 0;
}

static u16 rd_be16(const u8 *p)
{
	return (u16)(((u16)p[0] << 8) | p[1]);
}

static u32 rd_be32(const u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
	       ((u32)p[2] << 8)  | (u32)p[3];
}

static void wr_be16(u8 *p, u16 v)
{
	p[0] = (u8)(v >> 8);
	p[1] = (u8)v;
}

static void wr_be32(u8 *p, u32 v)
{
	p[0] = (u8)(v >> 24);
	p[1] = (u8)(v >> 16);
	p[2] = (u8)(v >> 8);
	p[3] = (u8)v;
}

/*
 * A session number that differs from the last one this box used.
 *
 * Not secrecy — there is nobody on this cable to guess it — but distinctness
 * across a reboot.  A peer that kept state from before the restart has to see
 * a number it has not seen, or it will read sequence 1 of the new session as
 * an ancient duplicate of the old one and drop the first thing said after
 * every reboot.  The tick count moves between boots and is the only thing here
 * that does; the counter beside it keeps two sockets opened in the same
 * millisecond apart.
 */
static u32 rel_next_session(void)
{
	static u32 bump;

	return (u32)get_jiffies() * 2654435761u + (++bump);
}

void rel_attach(struct rel_sock *r, struct udp_sock *udp)
{
	int i;

	r->udp          = udp;
	r->session      = rel_next_session();
	r->next_seq     = 1;
	r->in_flight    = 0;
	r->flight_seq   = 0;
	r->retries      = 0;
	r->peer_known   = 0;
	r->txq.head = r->txq.tail = 0;
	r->rxq.head = r->rxq.tail = 0;
	r->tx_msgs = r->retransmits = r->gave_up = 0;
	r->rx_msgs = r->rx_dups = r->rx_gaps = r->rx_malformed = 0;

	/*
	 * Published last: the transport task walks this table without a lock,
	 * and everything it will read has to be in place before the pointer it
	 * reads through appears.  Same shape as the ARP cache, same reason.
	 */
	for (i = 0; i < REL_MAX_SOCKS; i++) {
		if (!rel_socks[i]) {
			r->open = 1;
			rel_socks[i] = r;
			return;
		}
	}
	printk("[REL] no slot for a reliable socket\n");
}

void rel_detach(struct rel_sock *r)
{
	int i;

	/*
	 * Cleared before the slot, so the task can never find a socket it is
	 * allowed to service but whose udp_sock has been handed back.
	 */
	r->open = 0;
	for (i = 0; i < REL_MAX_SOCKS; i++)
		if (rel_socks[i] == r)
			rel_socks[i] = NULL;
}

int rel_send(struct rel_sock *r, const u8 *ip, u16 port,
	     const void *data, unsigned int len)
{
	const u8 *src = (const u8 *)data;
	struct rel_msg *m;
	unsigned int i;

	if (!r->open || len > REL_MSG_MAX)
		return -1;

	m = rel_tx_reserve(&r->txq);
	if (!m)
		return -1;		/* queue full; the caller retries */

	for (i = 0; i < IP_ALEN; i++)
		m->peer_ip[i] = ip[i];
	m->peer_port = port;
	m->len       = (u16)len;
	for (i = 0; i < len; i++)
		m->data[i] = src[i];

	rel_tx_commit(&r->txq);
	return 0;
}

int rel_recv(struct rel_sock *r, u8 *out_ip, u16 *out_port,
	     void *buf, unsigned int len)
{
	struct rel_msg *m = rel_rx_peek(&r->rxq);
	u8 *dst = (u8 *)buf;
	unsigned int n, i;

	if (!m)
		return 0;

	n = m->len < len ? m->len : len;
	for (i = 0; i < n; i++)
		dst[i] = m->data[i];
	for (i = 0; i < IP_ALEN; i++)
		out_ip[i] = m->peer_ip[i];
	*out_port = m->peer_port;

	rel_rx_release(&r->rxq);
	return (int)n;
}

/*
 * Put one DATA on the wire.  Returns 0 if it left, -1 if it did not.
 *
 * A -1 here is usually ARP still resolving, and is not treated as an error:
 * the caller leaves the message in flight and the ordinary retransmission
 * timer tries again.  An address that is not known yet and a datagram that was
 * lost are the same event to a sender, and giving them one recovery path is
 * why the first message to a new peer needs no special case anywhere above.
 */
static int rel_put_data(struct rel_sock *r, const struct rel_msg *m, u32 seq)
{
	u8 frame[REL_HDR_LEN + REL_MSG_MAX];
	unsigned int i;

	wr_be16(frame + REL_OFF_MAGIC, REL_MAGIC);
	frame[REL_OFF_TYPE] = REL_TYPE_DATA;
	frame[REL_OFF_RSVD] = 0;
	wr_be32(frame + REL_OFF_SESSION, r->session);
	wr_be32(frame + REL_OFF_SEQ, seq);

	for (i = 0; i < m->len; i++)
		frame[REL_HDR_LEN + i] = m->data[i];

	return udp_send_to(r->udp, m->peer_ip, m->peer_port,
			   frame, REL_HDR_LEN + m->len) == 0 ? 0 : -1;
}

/*
 * Acknowledge, using the address the datagram arrived from.
 *
 * udp_reply() rather than udp_send_to(): the frame that arrived carries the
 * link address it came from, so answering it needs no lookup and cannot fail
 * because ARP has not resolved.  An acknowledgement that could be delayed by
 * an address lookup would make the peer retransmit for a reason that has
 * nothing to do with the message.
 */
static void rel_put_ack(struct rel_sock *r, const struct udp_datagram *dg,
			u32 session, u32 seq)
{
	u8 frame[REL_HDR_LEN];

	wr_be16(frame + REL_OFF_MAGIC, REL_MAGIC);
	frame[REL_OFF_TYPE] = REL_TYPE_ACK;
	frame[REL_OFF_RSVD] = 0;
	wr_be32(frame + REL_OFF_SESSION, session);
	wr_be32(frame + REL_OFF_SEQ, seq);

	udp_reply(r->udp, dg, frame, REL_HDR_LEN);
}

static void rel_on_data(struct rel_sock *r, const struct udp_datagram *dg,
			u32 session, u32 seq)
{
	unsigned int len = dg->len - REL_HDR_LEN;
	struct rel_msg *m;
	int deliver;
	unsigned int i;

	if (len > REL_MSG_MAX) {
		r->rx_malformed++;
		return;
	}

	if (!r->peer_known || session != r->peer_session) {
		/*
		 * A stream this socket has not heard from, or the same peer
		 * after a restart.  Believe it and take its sequence as the
		 * starting point rather than insisting on 1: the peer may have
		 * been talking to somebody else first, and refusing would make
		 * a reboot look like a broken link.
		 */
		r->peer_known   = 1;
		r->peer_session = session;
		r->peer_seq     = seq;
		deliver = 1;
	} else if (seq == r->peer_seq + 1) {
		r->peer_seq = seq;
		deliver = 1;
	} else if ((long)(seq - r->peer_seq) <= 0) {
		/*
		 * Already delivered.  The acknowledgement was lost rather than
		 * the message, so it has to be sent again — dropping silently
		 * here is how a working link retransmits for ever.
		 */
		r->rx_dups++;
		deliver = 0;
	} else {
		/*
		 * Ahead of what is expected, which stop-and-wait should make
		 * impossible: the peer cannot send seq+2 before seq+1 is
		 * acknowledged.  Refuse it and say nothing, so the peer's own
		 * timer resends what is actually missing.  Acknowledging it
		 * would claim a message this socket never delivered.
		 */
		r->rx_gaps++;
		return;
	}

	if (deliver) {
		m = rel_rx_reserve(&r->rxq);
		if (!m) {
			/*
			 * Nowhere to put it, so it has not been delivered and
			 * must not be acknowledged — and @peer_seq has to go
			 * back, or the retransmission that follows will look
			 * like a duplicate and be acknowledged without ever
			 * reaching the application.
			 */
			if (seq == r->peer_seq)
				r->peer_seq = seq - 1;
			return;
		}
		for (i = 0; i < IP_ALEN; i++)
			m->peer_ip[i] = dg->src_ip[i];
		m->peer_port = dg->src_port;
		m->len       = (u16)len;
		for (i = 0; i < len; i++)
			m->data[i] = dg->data[REL_HDR_LEN + i];
		rel_rx_commit(&r->rxq);
		r->rx_msgs++;
	}

	rel_put_ack(r, dg, session, seq);
}

static void rel_on_ack(struct rel_sock *r, u32 session, u32 seq)
{
	/*
	 * Only the message actually in flight can be acknowledged, and only in
	 * this socket's own session.  A late acknowledgement from a previous
	 * session — the peer answering something sent before a restart — would
	 * otherwise retire a message that was never delivered.
	 */
	if (!r->in_flight || session != r->session || seq != r->flight_seq)
		return;

	rel_tx_release(&r->txq);
	r->in_flight = 0;
	r->retries   = 0;
	r->tx_msgs++;
}

static void rel_input(struct rel_sock *r)
{
	struct udp_datagram *dg;

	while ((dg = udp_poll(r->udp)) != NULL) {
		if (dg->len < REL_HDR_LEN ||
		    rd_be16(dg->data + REL_OFF_MAGIC) != REL_MAGIC) {
			r->rx_malformed++;
			udp_done(r->udp);
			continue;
		}

		u8  type    = dg->data[REL_OFF_TYPE];
		u32 session = rd_be32(dg->data + REL_OFF_SESSION);
		u32 seq     = rd_be32(dg->data + REL_OFF_SEQ);

		if (type == REL_TYPE_DATA)
			rel_on_data(r, dg, session, seq);
		else if (type == REL_TYPE_ACK)
			rel_on_ack(r, session, seq);
		else
			r->rx_malformed++;

		udp_done(r->udp);
	}
}

static void rel_output(struct rel_sock *r)
{
	struct rel_msg *m = rel_tx_peek(&r->txq);

	if (!m)
		return;

	if (!r->in_flight) {
		r->flight_seq = r->next_seq++;
		r->in_flight  = 1;
		r->retries    = 0;
		r->deadline   = get_jiffies() + REL_MS(REL_RETRY_MS);
		rel_put_data(r, m, r->flight_seq);
		return;
	}

	if (!rel_due(r->deadline))
		return;

	if (++r->retries > REL_RETRIES) {
		/*
		 * Given up.  The message is dropped rather than retried for
		 * ever, because a queue that never drains stops every later
		 * message behind it — a peer that is switched off would
		 * silence this socket permanently instead of for as long as it
		 * is away.
		 */
		r->gave_up++;
		rel_tx_release(&r->txq);
		r->in_flight = 0;
		r->retries   = 0;
		return;
	}

	r->retransmits++;
	r->deadline = get_jiffies() + REL_MS(REL_RETRY_MS);
	rel_put_data(r, m, r->flight_seq);
}

static void rel_task(void)
{
	for (;;) {
		for (int i = 0; i < REL_MAX_SOCKS; i++) {
			struct rel_sock *r = rel_socks[i];

			if (!r || !r->open)
				continue;
			rel_input(r);
			rel_output(r);
		}
		msleep(REL_TICK_MS);
	}
}

void rel_dump_stats(void)
{
	for (int i = 0; i < REL_MAX_SOCKS; i++) {
		struct rel_sock *r = rel_socks[i];

		if (!r || !r->open)
			continue;

		/*
		 * Retransmits and duplicates are printed next to the counts
		 * they qualify because either alone says nothing: a hundred
		 * messages with a hundred retransmits is a link that works and
		 * is losing half of everything, and looks identical to a
		 * healthy one if only the first number is shown.
		 */
		printk("[REL] :%lu out %lu (%lu resent, %lu given up),"
		       " in %lu (%lu dup, %lu gap, %lu bad)\n",
		       (unsigned long)r->udp->port,
		       r->tx_msgs, r->retransmits, r->gave_up,
		       r->rx_msgs, r->rx_dups, r->rx_gaps, r->rx_malformed);
	}
}

static int __init rel_init(void)
{
	struct task_struct *t = task_create(rel_task, PRIO_REL, "rel");

	if (!t) {
		printk("[REL] could not create the transport task\n");
		return -1;
	}

	/*
	 * task_create() builds a task; it does not make it runnable.  Leaving
	 * this out cost a round of flashing and looked like a protocol bug from
	 * every angle: the task appeared in ps, at the right priority, with its
	 * state printed as RUNNING — it just was never in the bitmap
	 * pick_next_task() scans, so it had never run.  PICKED = 0 next to an
	 * idle task picked thirty thousand times was the only thing that said so.
	 */
	enqueue_task(&runqueue, t);

	printk("[REL] reliable transport up, %d ms tick, %d ms retry x%d\n",
	       REL_TICK_MS, REL_RETRY_MS, REL_RETRIES);
	return 0;
}
device_initcall(rel_init);
