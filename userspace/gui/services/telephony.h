#ifndef __GUI_TELEPHONY_H
#define __GUI_TELEPHONY_H

/*
 * Telephony service — the state machine, call log, and actions behind the
 * Phone app. Screens/overlay talk only to this interface.
 *
 * The hardware boundary is mocked: an lv_timer drives the call lifecycle
 * (an outgoing call "connects" after a moment, an incoming call arrives
 * when idle, an unanswered ring becomes a missed call). When the SIM7600CE
 * modem lands, only the action functions and the tick become real AT
 * commands (ATD/ATA/ATH, +CLIP/RING URCs) — the state machine, call log,
 * and every screen stay exactly as they are.
 */

#define CALL_NUM_MAX   24
#define CALL_NAME_MAX  32

enum tel_state {
	TEL_IDLE,	/* no call */
	TEL_DIALING,	/* outgoing, waiting for the remote to pick up */
	TEL_INCOMING,	/* incoming, ringing locally */
	TEL_ACTIVE,	/* connected, duration counting */
};

enum call_type {
	CALL_OUTGOING,
	CALL_INCOMING,
	CALL_MISSED,
};

/* The two parties' view of the current call (name resolved, or NULL). */
struct call_info {
	const char *name;
	const char *number;
};

/* One entry in the recent-calls log. */
struct call_log_entry {
	char          number[CALL_NUM_MAX];
	char          name[CALL_NAME_MAX];   /* resolved at log time, "" if unknown */
	unsigned char type;                  /* enum call_type */
	unsigned int  dur_sec;               /* connected duration, 0 if never connected */
};

/* Notified on every state transition so the call overlay can re-render. */
typedef void (*tel_observer_fn)(enum tel_state state);

/* Load the call log and start the mock event timer. Call once at startup. */
void telephony_init(void);

/* Enable/disable the mock radio's auto-injected incoming calls (on by
 * default). Turn off to isolate pure-GUI testing. */
void telephony_set_mock(int on);

/* Wipe the call log and persist the empty state. Used by the soak build so
 * every run starts from an identical, clean call log (deterministic repro). */
void telephony_calllog_clear(void);

/* Actions. */
void telephony_dial(const char *number);   /* IDLE -> DIALING */
void telephony_answer(void);               /* INCOMING -> ACTIVE */
void telephony_reject(void);               /* INCOMING -> IDLE, logs a missed call */
void telephony_hangup(void);               /* any -> IDLE, logs the call */
void telephony_mute(int on);

/* State queries (for the overlay). */
enum tel_state            telephony_state(void);
const struct call_info   *telephony_current(void);
unsigned int              telephony_duration_sec(void);
int                       telephony_muted(void);
void                      telephony_set_observer(tel_observer_fn cb);

/* Recent-calls log. index 0 is the most recent. */
int                          telephony_log_count(void);
const struct call_log_entry *telephony_log_get(int index);

#endif
