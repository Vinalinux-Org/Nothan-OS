/*
 * services/telephony.c - Phone state machine, call log, and mock radio
 *
 * The state machine and call log are real; only the radio is mocked. An
 * lv_timer (telephony_tick) advances the call lifecycle on its own:
 *   - DIALING  -> ACTIVE   after DIAL_CONNECT_MS  (remote "answers")
 *   - ACTIVE   -> IDLE     after ACTIVE_HANGUP_MS (remote "hangs up")
 *   - INCOMING -> MISSED   after RING_TIMEOUT_MS  (nobody picks up)
 *   - IDLE     -> INCOMING every INCOMING_EVERY_MS (a call arrives)
 * Swapping in the SIM7600CE means replacing the actions with AT commands
 * and this tick with modem URC handling; nothing above this file changes.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "telephony.h"
#include "contacts.h"
#include "storage.h"
#include "modem_client.h"
#include "../core/log.h"
#include "lvgl/lvgl.h"

#define CALLLOG_MAX     32
#define CALLLOG_PATH    "/CALLLOG.BIN"
#define CALLLOG_MAGIC   0x474C4144u  /* "CALD" */

/* Mock timing (milliseconds). */
#define DIAL_CONNECT_MS    2500
#define ACTIVE_HANGUP_MS   8000		/* mock remote ends the call after this */
#define RING_TIMEOUT_MS    12000
#define INCOMING_EVERY_MS  30000
#define TICK_PERIOD_MS     500

struct calllog_blob {
	unsigned int          magic;
	unsigned int          count;
	struct call_log_entry entries[CALLLOG_MAX];
};

/* Current call. */
static enum tel_state  state = TEL_IDLE;
static char            cur_number[CALL_NUM_MAX];
static char            cur_name[CALL_NAME_MAX];   /* "" if unknown */
static struct call_info cur_info;
static int             cur_dir;                   /* enum call_type at pickup */
static int             muted;
static uint32_t        state_since;               /* lv_tick at last transition */
static uint32_t        active_since;              /* lv_tick when ACTIVE began */
static uint32_t        next_incoming_at;          /* lv_tick to inject next ring */

static tel_observer_fn observer;
static int             mock_on = 1;   /* auto-inject incoming calls */

/* Call log: append-only ring, newest at the end. */
static struct call_log_entry log_buf[CALLLOG_MAX];
static int                   log_n;

/* Mock incoming callers (for MONKEY builds only). */
static const char *mock_callers[] = { "0868 000 000" };
static int         mock_idx;

static void copy_str(char *dst, const char *src, int max)
{
	int i = 0;
	if (src) {
		for (; src[i] && i < max - 1; i++) {
			dst[i] = src[i];
		}
	}
	dst[i] = '\0';
}

static void calllog_save(void)
{
	struct calllog_blob blob;
	blob.magic = CALLLOG_MAGIC;
	blob.count = (unsigned int)log_n;
	for (int i = 0; i < log_n; i++) {
		blob.entries[i] = log_buf[i];
	}
	storage_write(CALLLOG_PATH, &blob, sizeof(blob));
}

static void calllog_load(void)
{
	struct calllog_blob blob;
	int n = storage_read(CALLLOG_PATH, &blob, sizeof(blob));
	if (n < (int)(sizeof(blob.magic) + sizeof(blob.count))) {
		return;
	}
	if (blob.magic != CALLLOG_MAGIC || blob.count > CALLLOG_MAX) {
		return;
	}
	log_n = (int)blob.count;
	for (int i = 0; i < log_n; i++) {
		log_buf[i] = blob.entries[i];
	}
}

/* Append an entry, dropping the oldest when full, then persist. */
static void calllog_add(const char *number, const char *name,
			enum call_type type, unsigned int dur_sec)
{
	if (log_n == CALLLOG_MAX) {
		for (int i = 0; i < CALLLOG_MAX - 1; i++) {
			log_buf[i] = log_buf[i + 1];
		}
		log_n--;
	}
	struct call_log_entry *e = &log_buf[log_n++];
	copy_str(e->number, number, CALL_NUM_MAX);
	copy_str(e->name, name, CALL_NAME_MAX);
	e->type    = (unsigned char)type;
	e->dur_sec = dur_sec;
	calllog_save();
}

/* Resolve a number to a saved contact name into cur_name (else ""). */
static void resolve_name(const char *number)
{
	const struct contact *c = contacts_find_by_phone(number);
	copy_str(cur_name, c ? c->name : "", CALL_NAME_MAX);
}

static void set_state(enum tel_state s)
{
	state = s;
	state_since = lv_tick_get();
	cur_info.name   = cur_name[0] ? cur_name : NULL;
	cur_info.number = cur_number;
	if (observer) {
		observer(s);
	}
}

static void go_idle(void)
{
	muted = 0;
	next_incoming_at = lv_tick_get() + INCOMING_EVERY_MS;
	set_state(TEL_IDLE);
}

void telephony_dial(const char *number)
{
	if (state != TEL_IDLE || !number || !number[0]) {
		return;
	}
	copy_str(cur_number, number, CALL_NUM_MAX);
	resolve_name(number);
	cur_dir = CALL_OUTGOING;
	gui_logf("telephony: dial %s\n", cur_number);
	set_state(TEL_DIALING);
	modem_cmd_dial(number);
}

void telephony_answer(void)
{
	if (state != TEL_INCOMING) {
		return;
	}
	gui_logf("telephony: answer %s\n", cur_name[0] ? cur_name : cur_number);
	active_since = lv_tick_get();
	set_state(TEL_ACTIVE);
	modem_cmd_answer();
}

void telephony_reject(void)
{
	if (state != TEL_INCOMING) {
		return;
	}
	gui_log("telephony: reject\n");
	calllog_add(cur_number, cur_name, CALL_MISSED, 0);
	go_idle();
	modem_cmd_reject();
}

void telephony_hangup(void)
{
	if (state == TEL_IDLE) {
		return;
	}
	gui_log("telephony: hangup\n");

	if (state == TEL_ACTIVE) {
		unsigned int dur = (lv_tick_get() - active_since) / 1000u;
		calllog_add(cur_number, cur_name,
			    cur_dir == CALL_OUTGOING ? CALL_OUTGOING : CALL_INCOMING,
			    dur);
	} else if (state == TEL_DIALING) {
		/* Cancelled before the remote answered. */
		calllog_add(cur_number, cur_name, CALL_OUTGOING, 0);
	} else { /* TEL_INCOMING hung up == rejected */
		calllog_add(cur_number, cur_name, CALL_MISSED, 0);
	}
	go_idle();
	modem_cmd_hangup();
}

void telephony_mute(int on)
{
	muted = on;
	gui_logf("telephony: mute %s\n", on ? "on" : "off");
	modem_cmd_mute(on);
}

/* Inject a mock incoming call (only when idle). */
static void inject_incoming(void)
{
	const char *num = mock_callers[mock_idx];
	mock_idx = (mock_idx + 1) % (int)(sizeof(mock_callers) / sizeof(mock_callers[0]));
	copy_str(cur_number, num, CALL_NUM_MAX);
	resolve_name(num);
	cur_dir = CALL_INCOMING;
	gui_logf("telephony: incoming %s\n", cur_name[0] ? cur_name : cur_number);
	set_state(TEL_INCOMING);
}

static void telephony_tick(lv_timer_t *t)
{
	(void)t;

	/* When mock is OFF the real modem backend drives the state machine
	 * through modem_client.c. The mock tick is inert so it never races
	 * with daemon URCs. */
	if (!mock_on)
		return;

	uint32_t now = lv_tick_get();

	switch (state) {
	case TEL_DIALING:
		if (now - state_since >= DIAL_CONNECT_MS) {
			gui_log("telephony: remote answered\n");
			active_since = now;
			set_state(TEL_ACTIVE);
		}
		break;
	case TEL_INCOMING:
		if (now - state_since >= RING_TIMEOUT_MS) {
			gui_log("telephony: missed (timeout)\n");
			calllog_add(cur_number, cur_name, CALL_MISSED, 0);
			go_idle();
		}
		break;
	case TEL_IDLE:
		if (mock_on && (int32_t)(now - next_incoming_at) >= 0) {
			inject_incoming();
		}
		break;
	case TEL_ACTIVE:
		/* Mock radio: the remote ends the call on its own so a soak run
		 * (or an unattended demo) is never stuck on the call screen. A
		 * real call only ends when the user hangs up (mock_on == 0). */
		if (mock_on && now - active_since >= ACTIVE_HANGUP_MS) {
			gui_log("telephony: remote hung up\n");
			telephony_hangup();
		}
		break;
	default:
		break;
	}
}

void telephony_init(void)
{
	calllog_load();
	next_incoming_at = lv_tick_get() + INCOMING_EVERY_MS;
	cur_info.name   = NULL;
	cur_info.number = cur_number;
	lv_timer_create(telephony_tick, TICK_PERIOD_MS, NULL);
}

enum tel_state telephony_state(void) { return state; }

const struct call_info *telephony_current(void) { return &cur_info; }

unsigned int telephony_duration_sec(void)
{
	if (state != TEL_ACTIVE) {
		return 0;
	}
	return (lv_tick_get() - active_since) / 1000u;
}

int telephony_muted(void) { return muted; }

void telephony_set_observer(tel_observer_fn cb) { observer = cb; }

void telephony_set_mock(int on) { mock_on = on; }

/* ─── Modem-client entry points (called from modem_client.c dispatch) ─── */

void telephony_on_incoming(const char *number)
{
    if (state != TEL_IDLE)
        return;
    copy_str(cur_number, number, CALL_NUM_MAX);
    resolve_name(number);
    cur_dir = CALL_INCOMING;
    gui_logf("telephony: incoming %s\n", cur_name[0] ? cur_name : cur_number);
    set_state(TEL_INCOMING);
}

void telephony_on_connected(void)
{
    if (state != TEL_DIALING && state != TEL_INCOMING)
        return;
    gui_log("telephony: remote answered\n");
    active_since = lv_tick_get();
    set_state(TEL_ACTIVE);
}

void telephony_on_remote_end(int is_missed)
{
    if (state == TEL_IDLE)
        return;
    gui_logf("telephony: remote end (state=%d missed=%d)\n", state, is_missed);
    if (state == TEL_ACTIVE && !is_missed) {
        unsigned int dur = (lv_tick_get() - active_since) / 1000u;
        calllog_add(cur_number, cur_name,
                    cur_dir == CALL_OUTGOING ? CALL_OUTGOING : CALL_INCOMING, dur);
    } else if (state == TEL_INCOMING && is_missed) {
        calllog_add(cur_number, cur_name, CALL_MISSED, 0);
    } else if (state == TEL_DIALING && !is_missed) {
        calllog_add(cur_number, cur_name, CALL_OUTGOING, 0);
    }
    go_idle();
}

/* Log a missed call directly, without touching state machine.
 * Used for CCWA callers: daemon sends CALL_MISS(num) after CALL_END has
 * already moved state to IDLE, so telephony_on_remote_end would early-return. */
void telephony_log_missed_direct(const char *number)
{
    char name[CALL_NAME_MAX];
    const struct contact *c = contacts_find_by_phone(number);
    copy_str(name, c ? c->name : "", sizeof(name));
    gui_logf("telephony: missed (ccwa) %s\n", name[0] ? name : number);
    calllog_add(number, name, CALL_MISSED, 0);
}

/* Mock-only: returns whether mock injectors are active (so the GUI client
 * can skip pumping when the mock is driving the state machine). */
int telephony_mock_on(void) { return mock_on; }

/* Clock the mock state machine one tick (called from modem_client when
 * the mock timer is active, so it integrates with the lv_timer path). */
void telephony_tick_from_client(void)
{
    telephony_tick(NULL);
}

void telephony_calllog_clear(void)
{
	log_n = 0;
	calllog_save();		/* persist empty so the next boot also starts clean */
}

int telephony_log_count(void) { return log_n; }

const struct call_log_entry *telephony_log_get(int index)
{
	if (index < 0 || index >= log_n) {
		return NULL;
	}
	return &log_buf[log_n - 1 - index]; /* 0 = newest */
}
