/*
 * net/icmp.c - answer a ping
 *
 * Echo requests only.  It is worth having on its own, before anything useful
 * runs over IP, because it is the one check a person can perform without any
 * software written for the occasion: someone types ping and either the box is
 * on the network or it is not.
 *
 * No ARP lookup is needed to reply.  The request arrived in a frame whose
 * source address is exactly where the answer has to go, so the reply is
 * addressed from what was received rather than from a table.  That is why
 * there is still no ARP cache — nothing here has yet had to find an address it
 * was not already told.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/netdev.h>
#include <nothan/ipv4.h>
#include <nothan/printk.h>

#define ICMP_ECHO_REQUEST	8
#define ICMP_ECHO_REPLY		0

#define ICMP_TYPE		0
#define ICMP_CODE		1
#define ICMP_CHECKSUM		2
#define ICMP_ID			4
#define ICMP_SEQ		6
#define ICMP_MIN_LEN		8

static unsigned long icmp_echoes;

/*
 * Built from the request, not from a blank frame.
 *
 * An echo reply is the request with two ends exchanged and one byte changed,
 * so copying it keeps everything already correct — identifier, sequence, and
 * the payload the sender will compare against what it sent — in agreement with
 * what arrived.  Filling those from memory would be several more chances to be
 * wrong about something the other end is checking.
 */
void icmp_input(struct netdev *dev, const u8 *frame,
		const u8 *iphdr, unsigned int iphdr_len,
		const u8 *payload, unsigned int payload_len)
{
	static u8 reply[ETH_FRAME_MAX];
	unsigned int total, i;
	u8 *rip, *ricmp;

	if (payload_len < ICMP_MIN_LEN)
		return;

	if (payload[ICMP_TYPE] != ICMP_ECHO_REQUEST)
		return;			/* replies and errors have no reader */

	total = ETH_HDR_SIZE + iphdr_len + payload_len;
	if (total > sizeof(reply))
		return;

	/* Ethernet: back to the sender of this frame, from us. */
	for (i = 0; i < ETH_ALEN; i++) {
		reply[i]            = frame[ETH_ALEN + i];
		reply[ETH_ALEN + i] = dev->mac[i];
	}
	reply[12] = ETH_P_IPV4 >> 8;
	reply[13] = ETH_P_IPV4 & 0xFF;

	rip   = reply + ETH_HDR_SIZE;
	ricmp = rip + iphdr_len;

	for (i = 0; i < iphdr_len; i++)
		rip[i] = iphdr[i];
	for (i = 0; i < payload_len; i++)
		ricmp[i] = payload[i];

	/* Source and destination change places; the rest of the header stands. */
	for (i = 0; i < IP_ALEN; i++) {
		rip[IP_DST + i] = iphdr[IP_SRC + i];
		rip[IP_SRC + i] = ipv4_addr()[i];
	}

	/*
	 * A fresh TTL, because the received one has been decremented on the way
	 * here and reflecting it would shorten every reply by the distance
	 * travelled — invisible on a direct cable, wrong everywhere else.
	 */
	rip[IP_TTL] = 64;

	rip[IP_CHECKSUM]     = 0;
	rip[IP_CHECKSUM + 1] = 0;
	{
		u16 sum = ip_checksum(rip, iphdr_len, 0);

		rip[IP_CHECKSUM]     = (u8)(sum >> 8);
		rip[IP_CHECKSUM + 1] = (u8)sum;
	}

	ricmp[ICMP_TYPE]         = ICMP_ECHO_REPLY;
	ricmp[ICMP_CHECKSUM]     = 0;
	ricmp[ICMP_CHECKSUM + 1] = 0;
	{
		u16 sum = ip_checksum(ricmp, payload_len, 0);

		ricmp[ICMP_CHECKSUM]     = (u8)(sum >> 8);
		ricmp[ICMP_CHECKSUM + 1] = (u8)sum;
	}

	icmp_echoes++;

	if (icmp_echoes <= 8 || (icmp_echoes & 63) == 0)
		printk("[ICMP] echo %lu from %lu.%lu.%lu.%lu,"
		       " %u bytes — replying\n",
		       icmp_echoes,
		       (unsigned long)iphdr[IP_SRC + 0],
		       (unsigned long)iphdr[IP_SRC + 1],
		       (unsigned long)iphdr[IP_SRC + 2],
		       (unsigned long)iphdr[IP_SRC + 3],
		       payload_len);

	if (dev->tx(dev, reply, total) == 0)
		netdev_stats.tx_frames++;
}
