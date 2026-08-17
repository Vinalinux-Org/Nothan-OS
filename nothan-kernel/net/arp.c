/*
 * net/arp.c - answer the question "who has this address"
 *
 * The smallest protocol that makes this board visible to another machine, and
 * the one that has to work before anything else can: a host that will not
 * answer ARP cannot be reached by IP at all, however complete the rest is.
 *
 * Only requests are answered and only for one address.  There is no cache
 * here, because nothing yet sends to an address it has to look up — a cache
 * with no reader would be code maintained on the strength of a plan.  It
 * arrives with the first transmit that needs it.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/printk.h>

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
 * This board's address, for now.
 *
 * Where an address really comes from is undecided and is not a networking
 * question: a box in a meeting room might be handed one by DHCP, might carry a
 * fixed one from its own settings, or might need neither if it only ever talks
 * to its pair.  Deciding that belongs with the box's configuration model, not
 * with the first protocol that happens to need four bytes.
 *
 * So this is a constant, stated as one, on the subnet the development laptop
 * uses.  Anything that pretended to be more than a constant would be inventing
 * a design nobody has agreed to.
 */
static const u8 my_ip[4] = { 10, 42, 0, 2 };

static unsigned long arp_requests;
static unsigned long arp_replies_sent;

static int ip_equal(const u8 *a, const u8 *b)
{
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

static u16 be16(const u8 *p)
{
	return ((u16)p[0] << 8) | p[1];
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
	u8 frame[ARP_FRAME_LEN];
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

	if (dev->tx(dev, frame, ARP_FRAME_LEN) == 0) {
		arp_replies_sent++;
		netdev_stats.tx_frames++;
	}
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

	if (be16(frame + ARP_OP) != ARP_OP_REQUEST)
		return;			/* replies have no reader yet */

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
	printk("[ARP] %lu requests seen, %lu replies sent; address"
	       " %lu.%lu.%lu.%lu\n",
	       arp_requests, arp_replies_sent,
	       (unsigned long)my_ip[0], (unsigned long)my_ip[1],
	       (unsigned long)my_ip[2], (unsigned long)my_ip[3]);
}
