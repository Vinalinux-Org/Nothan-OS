#ifndef _NOTHAN_VIDEO_NET_H
#define _NOTHAN_VIDEO_NET_H

/*
 * include/nothan/video_net.h - a frame, cut up small enough to send
 *
 * A 400x240 image is 192,000 bytes and the largest thing this link will carry
 * in one piece is 1472, so a frame crosses the wire as about 132 datagrams and
 * the receiver has to put them back together.  This header is what lets it:
 * every datagram says which frame it belongs to and where in that frame it
 * goes, so reassembly is a write at an offset rather than a queue and a sort.
 *
 * Self-describing on purpose.  Width and height ride in every datagram rather
 * than only in the first, because "only in the first" means a receiver that
 * misses one datagram out of a hundred and thirty-two does not know the shape
 * of the frame it is holding.  Sixteen bytes against 1456 of payload is one
 * per cent, and it buys a receiver that can join a stream already in progress
 * and be correct from its first datagram.
 *
 * Losing datagrams is expected, not exceptional, and nothing here asks for a
 * retransmission — by the time one arrived the frame would be two frames old.
 * §7.2 puts video on datagrams for exactly this reason.  What a receiver does
 * with a hole is its own decision; see the note on VIDEO_F_LAST.
 */

#include <nothan/types.h>

#define VIDEO_PORT		5004	/* control in, stream out */

#define VIDEO_MAGIC		0x4E56u	/* 'N','V' */
#define VIDEO_VERSION		1

/*
 * Last datagram of this frame.
 *
 * A hint, not a promise: it is the one datagram whose loss cannot be inferred
 * from anything else, so a receiver must also treat "a datagram from a newer
 * frame arrived" as the end of the older one.  Given that fallback the flag
 * earns its place by letting a complete frame be displayed immediately rather
 * than one frame later.
 */
#define VIDEO_F_LAST		0x01

/*
 * Byte offsets within the datagram.  Written out rather than expressed as a
 * struct because a struct here would be a promise about padding and byte order
 * that C does not make, and the wire does not care what the compiler thinks.
 * Everything multi-byte is big endian, as the rest of this stack is.
 */
#define VIDEO_H_MAGIC		0	/* u16 */
#define VIDEO_H_VERSION		2	/* u8  */
#define VIDEO_H_FLAGS		3	/* u8  */
#define VIDEO_H_SEQ		4	/* u32, frame number */
#define VIDEO_H_OFFSET		8	/* u32, byte offset into the frame */
#define VIDEO_H_WIDTH		12	/* u16, pixels */
#define VIDEO_H_HEIGHT		14	/* u16, pixels */
#define VIDEO_HDR_LEN		16

/*
 * Pixels per datagram, as many as fit.
 *
 * Not rounded down to a whole row.  Row-aligned chunks would make a lost
 * datagram show up as a clean horizontal band, which is prettier, but a row is
 * 800 bytes so it costs eighty per cent more datagrams — and the transmit cost
 * measured on this board is now 2.16 us per frame against 6.6 ns per byte,
 * which is to say the count matters and the size barely does.  Reassembly
 * writes at a byte offset and does not care where rows begin.
 */
#define VIDEO_CHUNK		(1472u - VIDEO_HDR_LEN)

/*
 * The control messages, on the same port the stream leaves from.
 *
 * A four byte tag and nothing else.  Subscribing is how the far end says where
 * to send, which is the same job the ARP-less protocols here have always done
 * by answering the frame that arrived — the difference is that a stream
 * outlives the datagram that asked for it, so the address is remembered.
 *
 * This is not the signalling a call will eventually have, but it is not a
 * stand-in for it either: something has to tell a box where its peer is, and
 * every design for that ends with an address arriving over the network.
 */
#define VIDEO_MSG_LEN		4
#define VIDEO_MSG_SUBSCRIBE	"VSUB"
#define VIDEO_MSG_STOP		"VSTP"

#endif /* _NOTHAN_VIDEO_NET_H */
