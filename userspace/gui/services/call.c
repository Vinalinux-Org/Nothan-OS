/*
 * services/call.c - the call state machine
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "call.h"
#include "chat.h"
#include "chat_net.h"
#include "../core/log.h"
#include "lvgl/lvgl.h"

/*
 * LVGL's tick, not the kernel's.
 *
 * call.c is a GUI service and has to compile on the host as well as the board;
 * getticks() is an SVC instruction and does not exist there.  lv_tick_elaps()
 * is the same millisecond count on both, and it handles the counter wrapping,
 * which a subtraction of raw ticks would get wrong once every forty-nine days
 * in a way nobody would ever reproduce.
 */

static enum call_state   state;
static int               peer_idx = -1;
static uint32_t          since_ms;
static call_observer_fn  observer;
static call_ring_fn      ring_handler;

static void enter(enum call_state s, int peer)
{
	state    = s;
	peer_idx = peer;
	since_ms = lv_tick_get();

	if (observer)
		observer(s);
}

static int send_ctl(int peer, unsigned char type)
{
	const struct chat_peer *p = chat_peer_get(peer);

	if (!p)
		return -1;
	return chat_net_send_ctl(p->ip, p->port, type);
}

void call_init(void)
{
	state    = CALL_IDLE;
	peer_idx = -1;
	observer = 0;
}

enum call_state call_state(void) { return state; }
int call_peer(void)              { return peer_idx; }

unsigned call_secs(void)
{
	if (state == CALL_IDLE)
		return 0;
	return (unsigned)(lv_tick_elaps((uint32_t)since_ms) / 1000);
}

void call_set_observer(call_observer_fn fn) { observer = fn; }
void call_set_ring_handler(call_ring_fn fn)  { ring_handler = fn; }

int call_dial(int peer_idx_in)
{
	const struct chat_peer *p = chat_peer_get(peer_idx_in);

	if (state != CALL_IDLE || !p)
		return -1;

	/*
	 * The state moves before the invite is acknowledged, and that is not
	 * optimism — the transport takes responsibility for delivering it, and
	 * a screen that waited for an acknowledgement would show nothing for as
	 * long as a retransmission takes.  What the caller sees is "calling",
	 * which is true from the moment they press the button.
	 */
	send_ctl(peer_idx_in, CHAT_MSG_INVITE);
	enter(CALL_CALLING, peer_idx_in);
	gui_logf("call: inviting %s\n", p->addr);
	return 0;
}

void call_accept(void)
{
	if (state != CALL_RINGING)
		return;

	send_ctl(peer_idx, CHAT_MSG_ACCEPT);
	gui_logf("call: accepted\n");
	enter(CALL_ACTIVE, peer_idx);
}

void call_reject(void)
{
	if (state != CALL_RINGING)
		return;

	send_ctl(peer_idx, CHAT_MSG_REJECT);
	gui_logf("call: rejected\n");
	enter(CALL_IDLE, -1);
}

void call_hangup(void)
{
	if (state == CALL_IDLE)
		return;

	/*
	 * BYE covers three different endings — giving up on an invite, hanging
	 * up mid-call, and cancelling a call that was still ringing — because
	 * to the far end they are one event: this side is no longer in the
	 * call.  Three messages would mean three cases to handle for a
	 * distinction the receiver cannot act on differently.
	 */
	send_ctl(peer_idx, CHAT_MSG_BYE);
	gui_logf("call: hung up after %u s\n", call_secs());
	enter(CALL_IDLE, -1);
}

static int peer_by_ip(const unsigned char *ip)
{
	for (int i = 0; i < chat_peer_count(); i++) {
		const struct chat_peer *p = chat_peer_get(i);

		if (p->ip[0] == ip[0] && p->ip[1] == ip[1] &&
		    p->ip[2] == ip[2] && p->ip[3] == ip[3])
			return i;
	}
	return -1;
}

void call_on_control(const unsigned char *ip, unsigned char type)
{
	int from = peer_by_ip(ip);

	if (from < 0) {
		gui_logf("call: control %u from unknown %u.%u.%u.%u\n",
			 type, ip[0], ip[1], ip[2], ip[3]);
		return;
	}

	switch (type) {
	case CHAT_MSG_INVITE:
		if (state != CALL_IDLE) {
			/*
			 * Busy.  Answered rather than ignored: silence would
			 * leave the caller watching a ringing screen for thirty
			 * seconds to learn something this box already knows.
			 */
			send_ctl(from, CHAT_MSG_REJECT);
			gui_logf("call: busy, rejected %d\n", from);
			return;
		}
		gui_logf("call: incoming from %d\n", from);
		enter(CALL_RINGING, from);
		if (ring_handler)
			ring_handler(from);
		break;

	case CHAT_MSG_ACCEPT:
		/*
		 * Only meaningful while this side is the one waiting, and only
		 * from the peer it is waiting on.  An ACCEPT from anyone else —
		 * or after the call already ended — is stale, and acting on it
		 * would put this box in a call the other end is not in.
		 */
		if (state == CALL_CALLING && from == peer_idx) {
			gui_logf("call: answered\n");
			enter(CALL_ACTIVE, from);
		}
		break;

	case CHAT_MSG_REJECT:
		if (state == CALL_CALLING && from == peer_idx) {
			gui_logf("call: declined\n");
			enter(CALL_IDLE, -1);
		}
		break;

	case CHAT_MSG_BYE:
		if (state != CALL_IDLE && from == peer_idx) {
			gui_logf("call: far end hung up\n");
			enter(CALL_IDLE, -1);
		}
		break;

	default:
		gui_logf("call: unknown control %u\n", type);
		break;
	}
}

void call_tick(void)
{
	if (state != CALL_CALLING && state != CALL_RINGING)
		return;

	if (call_secs() < CALL_RING_SECS)
		return;

	/*
	 * Nobody answered.  Both ends run this, so both leave the call at about
	 * the same moment without a message — but the caller sends BYE anyway,
	 * because "about the same moment" is not a guarantee and a stray
	 * ringing screen is worse than a redundant datagram.
	 */
	gui_logf("call: no answer after %d s\n", CALL_RING_SECS);
	if (state == CALL_CALLING)
		send_ctl(peer_idx, CHAT_MSG_BYE);
	enter(CALL_IDLE, -1);
}
