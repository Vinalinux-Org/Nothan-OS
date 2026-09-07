#ifndef __GUI_CHAT_H
#define __GUI_CHAT_H

/*
 * services/chat.h - conversations carried over IP, not over the radio
 *
 * Deliberately separate from services/messages.h, which is SMS.  They look
 * alike on screen and are nothing alike underneath: an SMS is addressed by
 * phone number and handed to a modem, and the network decides where it goes;
 * this is addressed by IP and handed to a socket, and nothing decides
 * anything — either end can speak first and both have to agree who they are
 * talking to.  Folding them into one store would mean one set of fields
 * carrying two meanings, and the first bug would be a message sent by the
 * wrong path to the right-looking name.
 *
 * The transport is chat_net.c, kept separate the way modem_client.c is kept
 * separate from messages.c: this file knows what a conversation is, that one
 * knows how a message travels.  Nothing here calls a syscall, which is what
 * lets the simulator compile it unchanged on a laptop.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#define CHAT_TEXT_MAX   160
#define CHAT_NAME_MAX   24
#define CHAT_ADDR_MAX   16	/* "255.255.255.255" and a NUL */
#define CHAT_MSG_MAX    32	/* messages kept per conversation */
#define CHAT_PEER_MAX   4

struct chat_message {
	char text[CHAT_TEXT_MAX];
	int  sent;			/* 1 = by us, 0 = by them */
};

struct chat_peer {
	char name[CHAT_NAME_MAX];
	char addr[CHAT_ADDR_MAX];	/* the printable form, for the screen */
	unsigned char  ip[4];		/* the same address, for the wire */
	unsigned short port;

	struct chat_message msg[CHAT_MSG_MAX];
	int  count;			/* messages held, oldest first */
	int  unread;
};

void chat_init(void);

int  chat_peer_count(void);
const struct chat_peer *chat_peer_get(int idx);

/* The newest message, or NULL — for the one-line preview in the peer list. */
const struct chat_message *chat_peer_last(int idx);

/*
 * Append @text to @idx as ours and hand it to the transport.
 *
 * Returns 0 if the message was stored, whether or not it reached the network:
 * the bubble appears as the user releases the button, and a peer that is
 * switched off is the transport's problem to keep retrying, not a reason to
 * refuse the message on screen.  -1 means the peer or the text is not usable.
 */
int  chat_send(int idx, const char *text);

/* Append @text to @idx as theirs. */
int  chat_receive(int idx, const char *text);

/*
 * Deliver a message that arrived from @ip.
 *
 * Matching by address rather than trusting a name in the payload: the sender
 * is whoever the datagram says it is, and that is the one field on the wire
 * this box did not have to be told.  A message from an address not in the
 * peer list is dropped and counted, not turned into a new conversation —
 * inventing peers from arriving traffic is how a chat window fills with
 * whatever else is on the segment.
 */
void chat_receive_from(const unsigned char *ip, const char *text);

/* Drain the transport into the store.  Call once per GUI loop. */
void chat_pump(void);

void chat_mark_read(int idx);

/*
 * Add a peer.  @ip is dotted-quad text; rejected if it does not parse, so a
 * typo becomes a refusal on the screen rather than an ARP request for an
 * address nobody meant.
 *
 * Return: the new index, or -1.
 */
int  chat_peer_add(const char *name, const char *ip);

/* This box's own address, for the profile tab.  A constant until something
 * hands the network configuration to userspace. */
const char *chat_self_addr(void);

#endif
