#ifndef _NOTHAN_ARP_H
#define _NOTHAN_ARP_H

/*
 * include/nothan/arp.h - turning an IP address into a link address
 *
 * arp.c used to answer questions and ask none, and said so: there was no cache
 * because nothing sent to an address it had to look up.  Every transmit in the
 * kernel was a reply, and a reply already knows where it goes — the frame that
 * arrived carried the link address with it.
 *
 * Chat is the first thing that speaks first.  Either end can type, so neither
 * can wait to be addressed before it has an address, and the assumption that
 * held the network path together up to here stops holding.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>

void arp_input(struct netdev *dev, const u8 *frame, unsigned int len);
void arp_dump_stats(void);

/**
 * arp_resolve() - look up the link address for @ip, without sleeping
 * @dev: the device the frame will go out of
 * @ip:  the address being asked about, IP_ALEN bytes
 * @mac: filled with ETH_ALEN bytes on success, untouched otherwise
 *
 * Does not block, and that is the whole shape of it.  A resolve that slept
 * would be called from inside a send, and netdev.h gives the caller of
 * tx_alloc() the link's only transmit buffer until it sends or aborts — so a
 * sleep in the middle hands that buffer to a task that is not running.  The
 * answer instead is either already here or not yet, and a caller that gets
 * "not yet" tries again later with nothing held.
 *
 * A miss is not a failure: it sends the request that will make the next call
 * succeed.  So the ordinary use is to call it, send if it answers, and drop or
 * queue the payload if it does not.  The second attempt a moment later is the
 * one that goes out.
 *
 * Return: 0 with @mac filled, or -1 if the address is not known yet.
 */
int arp_resolve(struct netdev *dev, const u8 *ip, u8 *mac);

#endif /* _NOTHAN_ARP_H */
