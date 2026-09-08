#ifndef __GUI_CALL_H
#define __GUI_CALL_H

/*
 * services/call.h - who is calling whom, and how far along
 *
 * The state machine is here and not in the kernel because every question it
 * answers is a product question: how long a phone rings before it gives up,
 * whether a second call can arrive during the first, what a person sees while
 * they wait.  The kernel supplies the one thing that is mechanism — a
 * transport that delivers in order and does not lose things — and §7.2 says so
 * in as many words: reliable is for chat *and call control*.
 *
 * The states are the same four telephony.h has, and deliberately: a video call
 * and a voice call are the same conversation about whether two people are
 * talking, and giving them different vocabularies would mean two ways to be in
 * a call and a screen that has to know which.
 *
 * ONE CALL AT A TIME.  An INVITE arriving while busy is answered REJECT
 * immediately.  Not a limitation to be lifted later — a box with one camera,
 * one panel and one person in front of it has nowhere to put a second call.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

enum call_state {
	CALL_IDLE,	/* nothing */
	CALL_CALLING,	/* we invited; waiting for the far end or the timeout */
	CALL_RINGING,	/* they invited; waiting for this person */
	CALL_ACTIVE,	/* both agreed */
};

/*
 * How long an unanswered invite stands.
 *
 * Thirty seconds because that is roughly how long a person will let a phone
 * ring before deciding nobody is there, and the same number has to be used at
 * both ends: the caller gives up and the ringing end stops ringing, with no
 * message between them saying so.  Making them differ would leave one side
 * ringing at a caller who has already left.
 */
#define CALL_RING_SECS	30

void call_init(void);

enum call_state call_state(void);
int  call_peer(void);		/* peer index, or -1 */
unsigned call_secs(void);	/* seconds in the current state */

/* Actions.  Each sends exactly one control message and moves one state. */
int  call_dial(int peer_idx);
void call_accept(void);
void call_reject(void);
void call_hangup(void);

/* A control message arrived from @ip.  Called by chat_net.c. */
void call_on_control(const unsigned char *ip, unsigned char type);

/* Once per GUI loop: the second-hand, and the ring timeout. */
void call_tick(void);

/*
 * Told whenever the state changes, so a screen can follow without polling.
 * One observer, because there is one call and one thing showing it.
 */
typedef void (*call_observer_fn)(enum call_state s);
void call_set_observer(call_observer_fn fn);

/*
 * Told when a call starts ringing, so something can put it on screen.
 *
 * Separate from the observer because they have different lifetimes: the
 * observer belongs to the call screen and exists only while it does, whereas
 * this one is set once at startup and must survive — an invite can arrive
 * while the person is anywhere in the system, and the only screen that could
 * have shown it is the one that does not exist yet.
 */
typedef void (*call_ring_fn)(int peer_idx);
void call_set_ring_handler(call_ring_fn fn);

#endif
