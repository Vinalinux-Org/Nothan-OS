#ifndef _NOTHAN_IPV4_H
#define _NOTHAN_IPV4_H

/*
 * include/nothan/ipv4.h - IPv4, and the address this board answers for
 *
 * The address lives here rather than in whichever protocol happened to need it
 * first.  ARP needs it to know which questions are about us; IP needs it to
 * know which packets are; anything later will need it too.  One definition,
 * one place to change when the answer to "where does an address come from"
 * finally gets decided.
 */

#include <nothan/types.h>
#include <nothan/netdev.h>

#define IP_ALEN			4

#define IPPROTO_ICMP		1
#define IPPROTO_UDP		17

/* Offsets from the start of the IPv4 header, not the frame. */
#define IP_VER_IHL		0
#define IP_TOS			1
#define IP_TOT_LEN		2
#define IP_ID			4
#define IP_FRAG			6
#define IP_TTL			8
#define IP_PROTO		9
#define IP_CHECKSUM		10
#define IP_SRC			12
#define IP_DST			16
#define IP_MIN_HDR		20

/* This board's address.  A constant, and said to be one — see ipv4.c. */
const u8 *ipv4_addr(void);

void ipv4_input(struct netdev *dev, const u8 *frame, unsigned int len);
void icmp_input(struct netdev *dev, const u8 *frame,
		const u8 *iphdr, unsigned int iphdr_len,
		const u8 *payload, unsigned int payload_len);

/*
 * The Internet checksum (RFC 1071): the one's complement of the one's
 * complement sum of 16-bit words.  Exposed because every layer above IP
 * computes its own over a different span, and three copies of a fold loop is
 * three chances to get the carry wrong.
 */
u16 ip_checksum(const u8 *data, unsigned int len, u32 seed);

#endif /* _NOTHAN_IPV4_H */
