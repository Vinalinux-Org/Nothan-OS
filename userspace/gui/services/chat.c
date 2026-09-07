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
#include "chat_net.h"
#include "../core/log.h"
#include "lvgl/lvgl.h"

static struct chat_peer peers[CHAT_PEER_MAX];
static int peer_count;
static unsigned long dropped_unknown;

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
	p->ip[0]  = 10; p->ip[1] = 42; p->ip[2] = 0; p->ip[3] = 1;
	p->port   = CHAT_PORT;
	p->count  = 0;
	p->unread = 0;

	chat_net_init();
}

/*
 * "10.42.0.1" into four bytes, or -1.
 *
 * Strict: an octet over 255, an empty one, or a fifth are all refused rather
 * than clamped.  A parser that guesses turns a typo into a working address for
 * a machine the user did not mean, and the mistake then surfaces two layers
 * down as ARP asking about a stranger.
 */
static int parse_ip(const char *s, unsigned char *ip)
{
	for (int i = 0; i < 4; i++) {
		int v = 0, digits = 0;

		while (*s >= '0' && *s <= '9') {
			v = v * 10 + (*s++ - '0');
			if (++digits > 3 || v > 255)
				return -1;
		}
		if (!digits)
			return -1;
		ip[i] = (unsigned char)v;

		if (i < 3) {
			if (*s != '.')
				return -1;
			s++;
		}
	}
	return *s ? -1 : 0;
}

int chat_peer_add(const char *name, const char *ip)
{
	unsigned char parsed[4];
	struct chat_peer *p;

	if (peer_count >= CHAT_PEER_MAX || !name || !name[0] || !ip)
		return -1;
	if (parse_ip(ip, parsed) != 0)
		return -1;

	p = &peers[peer_count];
	copy_str(p->name, name, CHAT_NAME_MAX);
	copy_str(p->addr, ip, CHAT_ADDR_MAX);
	for (int i = 0; i < 4; i++)
		p->ip[i] = parsed[i];
	p->port   = CHAT_PORT;
	p->count  = 0;
	p->unread = 0;

	gui_logf("chat: peer %s at %s\n", p->name, p->addr);
	return peer_count++;
}

/*
 * Our own address.
 *
 * A constant, and the same one net/ipv4.c holds — the kernel never tells
 * userspace what it configured, so this is a second copy of one fact and it
 * will drift the day the address becomes settable.  Worth the drift for now
 * because the profile screen showing nothing is worse than showing something
 * that has been true since the project started.
 */
const char *chat_self_addr(void)
{
	return "10.42.0.2";
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

	gui_logf("chat: to %s: %s\n", peers[idx].addr, m->text);
	chat_net_send(peers[idx].ip, peers[idx].port, m->text);
	return 0;
}

/*
 * Redraw whatever is on screen, from the LVGL task rather than from the pump.
 *
 * Same device sms_on_received() uses, and for the same reason: the pump runs
 * before lv_task_handler(), so rebuilding a screen from inside it would delete
 * objects while LVGL still holds pointers into them from the input it has not
 * finished dispatching.  Screens rebuild on SCREEN_LOADED; sending it by hand
 * is how a screen already loaded is told to look again.
 */
static void refresh_screen_cb(void *unused)
{
	(void)unused;
	lv_obj_send_event(lv_screen_active(), LV_EVENT_SCREEN_LOADED, NULL);
}

void chat_receive_from(const unsigned char *ip, const char *text)
{
	for (int i = 0; i < peer_count; i++) {
		const unsigned char *a = peers[i].ip;

		if (a[0] != ip[0] || a[1] != ip[1] ||
		    a[2] != ip[2] || a[3] != ip[3])
			continue;

		if (chat_receive(i, text) == 0) {
			gui_logf("chat: from %s: %s\n", peers[i].addr, text);
			lv_async_call(refresh_screen_cb, NULL);
		}
		return;
	}

	/*
	 * Nobody by that address.  Counted rather than ignored silently: on a
	 * segment where something else answers on this port, "no messages
	 * arrive" and "messages arrive from a stranger" look identical from the
	 * screen, and they are not the same problem.
	 */
	dropped_unknown++;
	gui_logf("chat: message from unknown %u.%u.%u.%u dropped (%lu so far)\n",
		 ip[0], ip[1], ip[2], ip[3], dropped_unknown);
}

void chat_pump(void)
{
	chat_net_pump();
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
