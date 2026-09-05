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
 * WHAT IS NOT HERE YET.  No socket.  chat_send() appends to the store and
 * returns; nothing reaches the wire.  That is the whole of what is missing,
 * and it is missing on purpose so the screens can be built and looked at
 * before the transport under them is wired in — the two fail differently and
 * finding out which is which is easier one at a time.  The seam is
 * chat_send() and chat_poll(); the kernel side they will call is already
 * done and tested (sock_open/sock_send/sock_recv over SOCK_RELIABLE).
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
	char addr[CHAT_ADDR_MAX];
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
 * Append @text to @idx as ours.  Returns 0, or -1 if the peer or the text is
 * not usable.  Sending will later mean queueing on the reliable socket; the
 * store is updated first either way, so the bubble appears as the user
 * releases the button rather than one round trip later.
 */
int  chat_send(int idx, const char *text);

/* Append @text to @idx as theirs.  For the receive path, and for the screens
 * to be testable before there is one. */
int  chat_receive(int idx, const char *text);

void chat_mark_read(int idx);

#endif
