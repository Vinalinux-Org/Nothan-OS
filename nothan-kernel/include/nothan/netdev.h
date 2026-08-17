#ifndef _NOTHAN_NETDEV_H
#define _NOTHAN_NETDEV_H

/*
 * include/nothan/netdev.h - the boundary between a link and everything above it
 *
 * os-architecture.md §7.1 names three seams that have to stay clean because
 * everything else stands on them, and this is one: above it nothing may know
 * whether the link is a wire, a radio or a modem.  That is not an abstraction
 * for its own sake — the chip this box ships on is not chosen (§14), the link
 * it ships with is not chosen either, and the work that takes the time is IP,
 * transport, audio and video, all of which sit above this line and none of
 * which should have to be revisited when the line below changes.
 *
 * Deliberately three things and no more: who we are, how to send, and a way in
 * for what arrives.  A driver that needs more than this is usually a driver
 * doing something above the line.
 *
 * Sending is three calls rather than one because of where the frame is built.
 * It used to be one — hand over a finished frame and the driver copies it — and
 * on this chip that copy went into CPPI RAM, which is device memory: 48.6 of
 * the 59.6 nanoseconds each transmitted byte cost were that one write, against
 * 11.0 for the whole receive path.  At video rates it was the difference
 * between spending a third of the machine on transmit and spending a
 * twentieth.  The copy is not removable while the caller owns the buffer, so
 * the buffer moved: the driver owns it, in DDR, and the caller builds the frame
 * where it will be sent from.
 *
 * One device.  A box with a single port does not need a list, and a list would
 * be the kind of generality that has to be maintained without ever being used.
 * When a second link exists — a radio beside the wire — this grows a lookup,
 * and that is a smaller change than the one saved by not writing it now.
 */

#include <nothan/types.h>

#define ETH_ALEN		6
#define ETH_HDR_SIZE		14
#define ETH_FRAME_MAX		1536

/* Ethertypes this kernel recognises. */
#define ETH_P_IPV4		0x0800
#define ETH_P_ARP		0x0806
#define ETH_P_IPV6		0x86DD

struct netdev {
	const char	*name;
	u8		mac[ETH_ALEN];

	/*
	 * Claim the transmit buffer.  Sleeps until one is free, so it must not
	 * be called from interrupt context.  Returns somewhere to build a frame
	 * of up to ETH_FRAME_MAX bytes, Ethernet header included.
	 */
	u8 *		(*tx_alloc)(struct netdev *dev);

	/*
	 * Send what was built, @len bytes of it, and give up the buffer.  Pads
	 * to the Ethernet minimum itself.  Returns 0 on success; on failure the
	 * buffer is released too, so a caller never has to unwind.
	 */
	int		(*tx_send)(struct netdev *dev, unsigned int len);

	/* Changed our mind after claiming.  Releases without sending. */
	void		(*tx_abort)(struct netdev *dev);
};

int netdev_register(struct netdev *dev);
struct netdev *netdev_get(void);

/*
 * The transmit path, wrapped so the frame counter lives in one place instead of
 * in every protocol that sends.
 *
 * THE RULE, and it is the whole cost of this seam: between alloc and
 * send-or-abort the caller holds the link's only transmit buffer, and every
 * other sender is asleep waiting for it.  So claim it once the frame is
 * decided on, not while still deciding — and never sleep on anything else in
 * between.  Three callers today; this comment is for the fourth.
 */
u8  *netdev_tx_alloc(struct netdev *dev);
int  netdev_tx_send(struct netdev *dev, unsigned int len);
void netdev_tx_abort(struct netdev *dev);

/*
 * A frame has arrived.  Called from the driver's receive *task*, never from
 * its interrupt handler — everything reachable from here is protocol work,
 * which kernel-roadmap.md §9.2 places outside an interrupt without exception.
 */
void netdev_rx(struct netdev *dev, const u8 *frame, unsigned int len);

/* Statistics, for anyone asking how the link is doing. */
struct netdev_stats {
	unsigned long	rx_frames;
	unsigned long	rx_unknown;	/* ethertype nothing handles */
	unsigned long	rx_short;	/* too small to be a frame */
	unsigned long	tx_frames;
};

extern struct netdev_stats netdev_stats;

/*
 * Print what every layer above the link has counted.
 *
 * One call rather than one per protocol, because a counter nothing prints is a
 * counter nobody reads: every protocol here keeps its own tally of what it
 * refused and why, and those are the numbers that tell a silent link apart from
 * a link full of packets being dropped for a reason.  net_core.c owns this for
 * the same reason it owns the demultiplex — it is the one file that knows which
 * protocols exist.
 */
void net_dump_stats(void);

#endif /* _NOTHAN_NETDEV_H */
