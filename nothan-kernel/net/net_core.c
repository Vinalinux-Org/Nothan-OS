/*
 * net/net_core.c - registration and the one place a frame is sorted by type
 *
 * Everything a driver hands up arrives here and leaves by exactly one of a
 * handful of doors, chosen by ethertype.  Keeping that decision in one
 * function rather than in each driver is what makes the netdev seam mean
 * something: a second link added later inherits the whole demultiplex by
 * calling netdev_rx(), and gets no say in it.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/printk.h>
#include <nothan/ipv4.h>

struct netdev_stats netdev_stats;

/*
 * PROTECTION: none needed.  Written once during probe, before any frame can
 * arrive, and read afterwards.  The app set is fixed at build time, so there
 * is no path that registers a device while the machine is running.
 */
static struct netdev *the_device;

int netdev_register(struct netdev *dev)
{
	if (!dev || !dev->tx)
		return -1;

	if (the_device) {
		printk("[NET] %s refused: %s is already registered\n",
		       dev->name, the_device->name);
		return -1;
	}

	the_device = dev;

	printk("[NET] %s registered, %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
	       dev->name,
	       (unsigned long)dev->mac[0], (unsigned long)dev->mac[1],
	       (unsigned long)dev->mac[2], (unsigned long)dev->mac[3],
	       (unsigned long)dev->mac[4], (unsigned long)dev->mac[5]);
	return 0;
}

struct netdev *netdev_get(void)
{
	return the_device;
}

void arp_input(struct netdev *dev, const u8 *frame, unsigned int len);
void arp_dump_stats(void);
void ipv4_dump_stats(void);

void net_dump_stats(void)
{
	printk("[NET] %lu frames in, %lu unhandled, %lu runt, %lu out\n",
	       netdev_stats.rx_frames, netdev_stats.rx_unknown,
	       netdev_stats.rx_short, netdev_stats.tx_frames);
	arp_dump_stats();
	ipv4_dump_stats();
}

void netdev_rx(struct netdev *dev, const u8 *frame, unsigned int len)
{
	u16 type;

	if (len < ETH_HDR_SIZE) {
		netdev_stats.rx_short++;
		return;
	}

	netdev_stats.rx_frames++;

	type = ((u16)frame[12] << 8) | frame[13];

	switch (type) {
	case ETH_P_ARP:
		arp_input(dev, frame, len);
		break;

	case ETH_P_IPV4:
		ipv4_input(dev, frame, len);
		break;

	/*
	 * Counted rather than ignored.  A link on a real network carries a
	 * steady stream of IPv6 and multicast that nothing here handles yet,
	 * and the difference between "nothing is arriving" and "plenty is
	 * arriving and none of it is for us" is one a log has to be able to
	 * state.
	 */
	default:
		netdev_stats.rx_unknown++;
		break;
	}
}
