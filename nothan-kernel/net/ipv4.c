/*
 * net/ipv4.c - enough IPv4 to be addressed, and no more
 *
 * Receive only: a packet arrives, is checked, and is handed to whatever
 * handles its protocol.  There is no routing table, no fragment reassembly and
 * no forwarding, because a box with one link has nowhere to route to and
 * nothing to forward for.  Those arrive if something ever needs them.
 *
 * Every field is validated before it is used.  A packet off the wire is the
 * only input to this machine that nobody here wrote, which os-architecture.md
 * §7.2 makes the rule for the whole network path: check the length before
 * reading, refuse and count, never trust a header.  Malformed input is an
 * ordinary event rather than a broken assumption, so it is dropped and
 * counted, never panicked on — §10.1 draws that line, and it is the line
 * between a box that survives a hostile network and one that does not.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/ipv4.h>
#include <nothan/printk.h>

/*
 * This board's address.
 *
 * Where an address really comes from is undecided and is not a networking
 * question: a box in a meeting room might be handed one by DHCP, might carry a
 * fixed one from its own settings, or might need neither if it only ever talks
 * to its pair.  That belongs with the box's configuration model.
 *
 * So: a constant, on the subnet the development laptop uses.  Anything that
 * pretended to be more would be inventing a design nobody has agreed to.
 */
static const u8 my_ip[IP_ALEN] = { 10, 42, 0, 2 };

/*
 * Two counters, because "arrived" and "was for us" are two facts and one
 * number cannot hold both.  ip_seen is every well formed packet on the wire;
 * ip_delivered is the subset that reached a handler.  They were one counter
 * once, labelled "for us", and it read 16 on a board that had received nothing
 * addressed to it — a counter that lies is worse than no counter at all.
 */
static unsigned long ip_seen;
static unsigned long ip_delivered;
static unsigned long ip_bad_header;
static unsigned long ip_bad_checksum;
static unsigned long ip_not_ours;
static unsigned long ip_no_handler;

const u8 *ipv4_addr(void)
{
	return my_ip;
}

u16 ip_checksum(const u8 *data, unsigned int len, u32 sum)
{
	unsigned int i;

	for (i = 0; i + 1 < len; i += 2)
		sum += ((u32)data[i] << 8) | data[i + 1];

	/* An odd trailing byte is the high half of a zero-padded word. */
	if (i < len)
		sum += (u32)data[i] << 8;

	/* Fold twice: the first fold can itself carry out. */
	while (sum >> 16)
		sum = (sum & 0xFFFFu) + (sum >> 16);

	return (u16)~sum;
}

void ipv4_input(struct netdev *dev, const u8 *frame, unsigned int len)
{
	const u8 *ip;
	unsigned int avail, ihl, tot_len;

	if (len < ETH_HDR_SIZE + IP_MIN_HDR) {
		ip_bad_header++;
		return;
	}

	ip    = frame + ETH_HDR_SIZE;
	avail = len - ETH_HDR_SIZE;

	if ((ip[IP_VER_IHL] >> 4) != 4) {
		ip_bad_header++;
		return;
	}

	ihl = (ip[IP_VER_IHL] & 0x0F) * 4u;
	if (ihl < IP_MIN_HDR || ihl > avail) {
		ip_bad_header++;
		return;
	}

	tot_len = ((unsigned int)ip[IP_TOT_LEN] << 8) | ip[IP_TOT_LEN + 1];

	/*
	 * The wire may carry more than the packet claims — Ethernet pads short
	 * frames to sixty bytes — but never less.  Trusting tot_len without
	 * this check is how a length field becomes a read past the buffer.
	 */
	if (tot_len < ihl || tot_len > avail) {
		ip_bad_header++;
		return;
	}

	if (ip_checksum(ip, ihl, 0) != 0) {
		ip_bad_checksum++;
		return;
	}

	ip_seen++;

	/*
	 * Fragments are dropped rather than reassembled.  Nothing here sends
	 * anything large enough to be fragmented on the way back, and a
	 * reassembly buffer is a well known way to be made to hold state on
	 * behalf of a stranger.  The more-fragments bit and a non-zero offset
	 * both mean the same thing here.
	 */
	if ((((unsigned int)ip[IP_FRAG] << 8) | ip[IP_FRAG + 1]) & 0x3FFF) {
		ip_bad_header++;
		return;
	}

	if (ip[IP_DST + 0] != my_ip[0] || ip[IP_DST + 1] != my_ip[1] ||
	    ip[IP_DST + 2] != my_ip[2] || ip[IP_DST + 3] != my_ip[3]) {
		ip_not_ours++;
		return;
	}

	switch (ip[IP_PROTO]) {
	case IPPROTO_ICMP:
		ip_delivered++;
		icmp_input(dev, frame, ip, ihl, ip + ihl, tot_len - ihl);
		break;

	case IPPROTO_UDP:
		ip_delivered++;
		udp_input(dev, frame, ip, ihl, ip + ihl, tot_len - ihl);
		break;

	default:
		ip_no_handler++;
		break;
	}
}

void ipv4_dump_stats(void)
{
	printk("[IP] %lu seen, %lu to us, %lu elsewhere; bad %lu hdr /"
	       " %lu csum, %lu no proto\n",
	       ip_seen, ip_delivered, ip_not_ours, ip_bad_header,
	       ip_bad_checksum, ip_no_handler);
}
