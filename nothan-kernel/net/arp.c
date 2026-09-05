/*
 * net/arp.c - answer the question "who has this address"
 *
 * The smallest protocol that makes this board visible to another machine, and
 * the one that has to work before anything else can: a host that will not
 * answer ARP cannot be reached by IP at all, however complete the rest is.
 *
 * The cache said it would arrive with the first transmit that needed it, and
 * this is that transmit: chat speaks first, so it has to address a machine
 * that has not addressed it.  Everything the kernel sent before now was an
 * answer, and an answer already knows where it goes.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/printk.h>
#include <nothan/ipv4.h>
#include <nothan/arp.h>
#include <nothan/time.h>

#define ARP_HDR_LEN		28
#define ARP_FRAME_LEN		(ETH_HDR_SIZE + ARP_HDR_LEN)

#define ARP_HTYPE_ETHERNET	1
#define ARP_OP_REQUEST		1
#define ARP_OP_REPLY		2

/* Offsets from the start of the frame, so the Ethernet header is included. */
#define ARP_HTYPE		14
#define ARP_PTYPE		16
#define ARP_HLEN		18
#define ARP_PLEN		19
#define ARP_OP			20
#define ARP_SENDER_MAC		22
#define ARP_SENDER_IP		28
#define ARP_TARGET_MAC		32
#define ARP_TARGET_IP		38

/*
 * The address lives in the IP layer, not here.
 *
 * It was defined in this file when ARP was the only protocol that needed it,
 * and IP needing it too is exactly the moment a second copy would have been
 * made — which is the shape that has cost this project six separate bugs in a
 * week.  One definition, in nothan/ipv4.h, and everything asks it.
 */
#define my_ip	ipv4_addr()

static unsigned long arp_requests;
static unsigned long arp_replies_sent;
static unsigned long arp_requests_sent;
static unsigned long arp_learned;
static unsigned long arp_evicted;

/*
 * Eight entries, because this box talks to a peer and a viewer and has no
 * router to reach past.  The number is small enough that a miss walks the
 * whole table and large enough that nothing on a desk-sized LAN evicts a
 * conversation that is still going.  If a real deployment ever thrashes it the
 * eviction counter below will say so, which is the only reason to change it.
 */
#define ARP_CACHE_SIZE		8

/*
 * How long an answer is believed, and how often an unanswered question is
 * repeated.
 *
 * A minute is short enough that a machine which changed its card is not
 * unreachable for the length of a meeting, and long enough that a running call
 * never pauses to ask again.  The retry interval matters more: without it a
 * task that sends every frame would broadcast an ARP request every frame while
 * the peer was down, which is a flood produced by a failure to talk to anyone.
 */
#define ARP_VALID_MS		60000
#define ARP_RETRY_MS		1000

enum arp_state {
	ARP_FREE = 0,
	ARP_PENDING,		/* asked, no answer yet */
	ARP_VALID,
};

struct arp_entry {
	u8		ip[IP_ALEN];
	u8		mac[ETH_ALEN];
	u8		state;
	unsigned long	stamp;		/* jiffies: learned, or last asked */
};

/*
 * PROTECTION: none, and it needs saying because this table has two writers
 * where the rest of the network path has one.  arp_input() runs in the
 * driver's receive task; arp_resolve() runs in whichever task is sending.
 *
 * What makes that safe is not luck but the size of the write: an entry is
 * published by storing @state last, and every field it guards is written
 * before that store.  A reader either sees the old state and ignores the
 * entry, or sees the new one and reads fields that were already there.  Single
 * core, no reordering visible to another task, and no torn read of a byte.
 *
 * This stops being true on a second core, and the roadmap keeps this box
 * single-core deliberately.  If that ever changes, this comment is the thing
 * that has to be revisited, not the code below.
 */
static struct arp_entry arp_cache[ARP_CACHE_SIZE];

static int ip_equal(const u8 *a, const u8 *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

static u16 be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
}

/* Milliseconds as the scheduler counts them, from the one place HZ is set. */
#define ARP_MS(ms)	((unsigned long)(ms) * HZ / 1000)

/*
 * Elapsed time, written so a wrapping jiffy counter cannot make an old entry
 * look fresh.  The subtraction wraps with the counter and the signed compare
 * reads the difference, which is the trick every kernel uses and the reason
 * neither side is compared directly.
 */
static int arp_older_than(unsigned long stamp, unsigned long ms)
{
	return (long)(get_jiffies() - stamp) >= (long)ARP_MS(ms);
}

static struct arp_entry *arp_find(const u8 *ip)
{
	for (int i = 0; i < ARP_CACHE_SIZE; i++)
		if (arp_cache[i].state != ARP_FREE &&
		    ip_equal(arp_cache[i].ip, ip))
			return &arp_cache[i];
	return NULL;
}

/*
 * A slot for an address not in the table: a free one, else the oldest.
 *
 * Oldest rather than least-recently-used, because @stamp is already kept for
 * ageing and an LRU would need a second timestamp updated on every lookup —
 * a write on the read path, to order eight entries.
 */
static struct arp_entry *arp_slot(void)
{
	struct arp_entry *oldest = &arp_cache[0];

	for (int i = 0; i < ARP_CACHE_SIZE; i++) {
		if (arp_cache[i].state == ARP_FREE)
			return &arp_cache[i];
		if ((long)(arp_cache[i].stamp - oldest->stamp) < 0)
			oldest = &arp_cache[i];
	}

	arp_evicted++;
	return oldest;
}

/*
 * arp_learn() - record what a frame said about its sender
 * @create: insert if not already known, rather than only refreshing
 *
 * Every ARP frame carries its sender's pair, request and reply alike, so the
 * answer to a question often arrives before it is asked.  Taking it is free.
 *
 * Taking it from *any* frame is not, which is what @create separates.  RFC 826
 * refreshes an entry already held no matter who the frame was addressed to,
 * but only creates one when the frame was addressed to us.  Without that
 * split, a busy LAN fills eight slots with machines this box will never speak
 * to, and evicts the peer it is in a call with — a table emptied by traffic
 * that was none of its business.
 */
static void arp_learn(const u8 *ip, const u8 *mac, int create)
{
	struct arp_entry *e;

	/*
	 * 0.0.0.0 is a probe asking whether an address is free, not a claim to
	 * own it.  Caching it would answer a later lookup with a machine that
	 * never said it was there.
	 */
	if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
		return;

	e = arp_find(ip);
	if (!e) {
		if (!create)
			return;
		e = arp_slot();
		for (int i = 0; i < IP_ALEN; i++)
			e->ip[i] = ip[i];
	}

	for (int i = 0; i < ETH_ALEN; i++)
		e->mac[i] = mac[i];
	e->stamp = get_jiffies();
	e->state = ARP_VALID;		/* published last, see the note above */
	arp_learned++;
}

/*
 * Ask the wire who owns @ip.
 *
 * Built from nothing rather than from a received frame, because there is no
 * received frame — this is the direction arp.c did not have.  Target hardware
 * address is zero: it is the field being asked about, and the broadcast in the
 * Ethernet header is what carries the question to whoever can answer.
 */
static void arp_request(struct netdev *dev, const u8 *ip)
{
	u8 *frame = netdev_tx_alloc(dev);
	unsigned int i;

	for (i = 0; i < ETH_ALEN; i++) {
		frame[i]            = 0xFF;
		frame[ETH_ALEN + i] = dev->mac[i];
	}
	frame[12] = (u8)(ETH_P_ARP >> 8);
	frame[13] = (u8)ETH_P_ARP;

	frame[ARP_HTYPE]     = 0;
	frame[ARP_HTYPE + 1] = ARP_HTYPE_ETHERNET;
	frame[ARP_PTYPE]     = (u8)(ETH_P_IPV4 >> 8);
	frame[ARP_PTYPE + 1] = (u8)ETH_P_IPV4;
	frame[ARP_HLEN]      = ETH_ALEN;
	frame[ARP_PLEN]      = IP_ALEN;
	frame[ARP_OP]        = 0;
	frame[ARP_OP + 1]    = ARP_OP_REQUEST;

	for (i = 0; i < ETH_ALEN; i++) {
		frame[ARP_SENDER_MAC + i] = dev->mac[i];
		frame[ARP_TARGET_MAC + i] = 0;
	}
	for (i = 0; i < IP_ALEN; i++) {
		frame[ARP_SENDER_IP + i] = my_ip[i];
		frame[ARP_TARGET_IP + i] = ip[i];
	}

	if (netdev_tx_send(dev, ARP_FRAME_LEN) == 0)
		arp_requests_sent++;
}

int arp_resolve(struct netdev *dev, const u8 *ip, u8 *mac)
{
	struct arp_entry *e;

	if (!dev)
		return -1;

	e = arp_find(ip);

	if (e && e->state == ARP_VALID && !arp_older_than(e->stamp, ARP_VALID_MS)) {
		for (int i = 0; i < ETH_ALEN; i++)
			mac[i] = e->mac[i];
		return 0;
	}

	/*
	 * Unknown, or known and stale.  Either way the answer has to be asked
	 * for — but at most once per retry interval, so that a caller sending
	 * at frame rate to a machine that is switched off produces one question
	 * a second rather than a broadcast storm.
	 *
	 * @stamp carries both meanings, "when this was learned" and "when this
	 * was last asked", and only one state can be in force at a time: a
	 * PENDING entry is never read for its address, so refreshing its stamp
	 * to pace the retries cannot make a stale address look fresh.  That is
	 * why an expired entry is demoted here rather than left VALID — leaving
	 * it would let the retry pacing keep resetting the expiry clock, and the
	 * address would then never be believed to be old again.
	 */
	if (!e) {
		e = arp_slot();
		for (int i = 0; i < IP_ALEN; i++)
			e->ip[i] = ip[i];
		/* Without this the entry stays FREE, arp_find() keeps skipping
		 * it, every retry takes another slot, and the pending count
		 * this table is meant to expose can never leave zero. */
		e->state = ARP_PENDING;
	} else if (e->state == ARP_VALID) {
		e->state = ARP_PENDING;
	} else if (!arp_older_than(e->stamp, ARP_RETRY_MS)) {
		return -1;		/* asked recently, still waiting */
	}

	e->stamp = get_jiffies();
	arp_request(dev, ip);
	return -1;
}

/*
 * Build the answer in place of the question.
 *
 * The reply is the request with the two parties exchanged: our address becomes
 * the sender, theirs becomes the target, and the operation changes.  Building
 * it from the received frame rather than from scratch keeps every field that
 * was already correct — the hardware and protocol type, the lengths — in
 * agreement with what the asker used, which is one fewer thing to get wrong
 * than filling a blank frame from memory.
 */
static void arp_reply(struct netdev *dev, const u8 *req)
{
	u8 *frame = netdev_tx_alloc(dev);
	unsigned int i;

	for (i = 0; i < ARP_FRAME_LEN; i++)
		frame[i] = req[i];

	/* Ethernet: back to whoever asked, from us. */
	for (i = 0; i < ETH_ALEN; i++) {
		frame[i]             = req[ETH_ALEN + i];
		frame[ETH_ALEN + i]  = dev->mac[i];
	}

	frame[ARP_OP]     = 0;
	frame[ARP_OP + 1] = ARP_OP_REPLY;

	/* Sender becomes us; target becomes the asker. */
	for (i = 0; i < ETH_ALEN; i++) {
		frame[ARP_TARGET_MAC + i] = req[ARP_SENDER_MAC + i];
		frame[ARP_SENDER_MAC + i] = dev->mac[i];
	}
	for (i = 0; i < 4; i++) {
		frame[ARP_TARGET_IP + i] = req[ARP_SENDER_IP + i];
		frame[ARP_SENDER_IP + i] = my_ip[i];
	}

	if (netdev_tx_send(dev, ARP_FRAME_LEN) == 0)
		arp_replies_sent++;
}

void arp_input(struct netdev *dev, const u8 *frame, unsigned int len)
{
	const u8 *sip, *tip;

	/*
	 * Every field is checked before it is used, because a frame off the
	 * wire is the one input to this machine that nobody here wrote.
	 * os-architecture.md §7.2 makes that the rule for the whole network
	 * path: check the length before reading, refuse and count, never trust
	 * a header.  A malformed frame is an ordinary event, not a broken
	 * assumption — so it is dropped, not panicked on.
	 */
	if (len < ARP_FRAME_LEN)
		return;

	if (be16(frame + ARP_HTYPE) != ARP_HTYPE_ETHERNET ||
	    be16(frame + ARP_PTYPE) != ETH_P_IPV4 ||
	    frame[ARP_HLEN] != ETH_ALEN ||
	    frame[ARP_PLEN] != 4)
		return;

	sip = frame + ARP_SENDER_IP;
	tip = frame + ARP_TARGET_IP;

	/*
	 * Learn before deciding whether to answer, and learn from replies too.
	 *
	 * A reply is the answer to a question this box asked, so it is the
	 * whole point of the cache; a request is a machine announcing its own
	 * pair on the way to asking about someone else's, and taking that costs
	 * nothing.  Insert only when the frame was addressed to us — see
	 * arp_learn() for why that split is not an optimisation.
	 */
	arp_learn(sip, frame + ARP_SENDER_MAC, ip_equal(tip, my_ip));

	if (be16(frame + ARP_OP) != ARP_OP_REQUEST)
		return;			/* a reply is now fully handled above */

	arp_requests++;

	if (!ip_equal(tip, my_ip))
		return;			/* asking about someone else */

	printk("[ARP] who has %lu.%lu.%lu.%lu — that is us; telling"
	       " %lu.%lu.%lu.%lu at %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
	       (unsigned long)tip[0], (unsigned long)tip[1],
	       (unsigned long)tip[2], (unsigned long)tip[3],
	       (unsigned long)sip[0], (unsigned long)sip[1],
	       (unsigned long)sip[2], (unsigned long)sip[3],
	       (unsigned long)frame[ARP_SENDER_MAC + 0],
	       (unsigned long)frame[ARP_SENDER_MAC + 1],
	       (unsigned long)frame[ARP_SENDER_MAC + 2],
	       (unsigned long)frame[ARP_SENDER_MAC + 3],
	       (unsigned long)frame[ARP_SENDER_MAC + 4],
	       (unsigned long)frame[ARP_SENDER_MAC + 5]);

	arp_reply(dev, frame);
}

void arp_dump_stats(void)
{
	int valid = 0, pending = 0;

	for (int i = 0; i < ARP_CACHE_SIZE; i++) {
		if (arp_cache[i].state == ARP_VALID)
			valid++;
		else if (arp_cache[i].state == ARP_PENDING)
			pending++;
	}

	printk("[ARP] %lu requests seen, %lu replies sent; address"
	       " %lu.%lu.%lu.%lu\n",
	       arp_requests, arp_replies_sent,
	       (unsigned long)my_ip[0], (unsigned long)my_ip[1],
	       (unsigned long)my_ip[2], (unsigned long)my_ip[3]);

	/*
	 * Asked-but-never-answered is the shape every failure here takes: a
	 * peer that is off, a wrong address, a cable in the wrong socket all
	 * look like pending entries that never become valid.  Printing the two
	 * counts apart is what tells those from a cache that is simply idle.
	 */
	printk("[ARP] cache %d valid, %d pending of %d; %lu asked, %lu learned,"
	       " %lu evicted\n",
	       valid, pending, ARP_CACHE_SIZE,
	       arp_requests_sent, arp_learned, arp_evicted);
}
