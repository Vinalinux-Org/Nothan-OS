/*
 * services/chat.c - the store behind the Chat screens
 *
 * Fixed arrays, no allocation.  A conversation holds the last CHAT_MSG_MAX
 * messages and drops the oldest when it fills, which is the behaviour a chat
 * window has anyway once it is longer than the screen — and it means a peer
 * that never stops talking cannot exhaust anything.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "chat.h"
#include "../core/log.h"

static struct chat_peer peers[CHAT_PEER_MAX];
static int peer_count;

static void copy_str(char *dst, const char *src, int max)
{
	int i = 0;

	while (i < max - 1 && src[i]) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

/*
 * Add a message, discarding the oldest if the conversation is full.
 *
 * The shift is a copy of up to 32 structs and happens once per message
 * arriving, which is a rate set by a person typing.  A ring with a head index
 * would avoid it and would also mean every screen that walks the thread has
 * to know where the start is; at this size the copy is the cheaper of the two
 * in the only currency that matters here, which is how many places can get the
 * index wrong.
 */
static struct chat_message *push(struct chat_peer *p)
{
	if (p->count == CHAT_MSG_MAX) {
		for (int i = 1; i < CHAT_MSG_MAX; i++)
			p->msg[i - 1] = p->msg[i];
		p->count = CHAT_MSG_MAX - 1;
	}
	return &p->msg[p->count++];
}

void chat_init(void)
{
	/*
	 * One peer, hard-coded, and it is the laptop.
	 *
	 * Not a placeholder for a contact list — a placeholder for an address.
	 * Two boxes finding each other needs something to announce and something
	 * to listen, and that is a piece of design in its own right, not a
	 * detail of this screen.  Until then the demo has exactly the peer it
	 * has always had, at the address every test tool in tests/net uses.
	 */
	peer_count = 0;

	struct chat_peer *p = &peers[peer_count++];
	copy_str(p->name, "Laptop", CHAT_NAME_MAX);
	copy_str(p->addr, "10.42.0.1", CHAT_ADDR_MAX);
	p->port   = 9999;
	p->count  = 0;
	p->unread = 0;
}

int chat_peer_count(void)
{
	return peer_count;
}

const struct chat_peer *chat_peer_get(int idx)
{
	if (idx < 0 || idx >= peer_count)
		return 0;
	return &peers[idx];
}

const struct chat_message *chat_peer_last(int idx)
{
	if (idx < 0 || idx >= peer_count || peers[idx].count == 0)
		return 0;
	return &peers[idx].msg[peers[idx].count - 1];
}

int chat_send(int idx, const char *text)
{
	struct chat_message *m;

	if (idx < 0 || idx >= peer_count || !text || !text[0])
		return -1;

	m = push(&peers[idx]);
	copy_str(m->text, text, CHAT_TEXT_MAX);
	m->sent = 1;

	/*
	 * Where the socket goes.  Everything above this line is what the screen
	 * needs; everything below it, once it exists, is what the peer needs.
	 */
	gui_logf("chat: to %s: %s\n", peers[idx].addr, m->text);
	return 0;
}

int chat_receive(int idx, const char *text)
{
	struct chat_message *m;

	if (idx < 0 || idx >= peer_count || !text || !text[0])
		return -1;

	m = push(&peers[idx]);
	copy_str(m->text, text, CHAT_TEXT_MAX);
	m->sent = 0;
	peers[idx].unread++;
	return 0;
}

void chat_mark_read(int idx)
{
	if (idx >= 0 && idx < peer_count)
		peers[idx].unread = 0;
}
