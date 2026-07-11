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

#ifdef GUI_MONKEY
/* Mock timing (milliseconds). */
#define DIAL_CONNECT_MS    2500
#define ACTIVE_HANGUP_MS   8000		/* mock remote ends the call after this */
#define RING_TIMEOUT_MS    12000
#define INCOMING_EVERY_MS  30000
#define TICK_PERIOD_MS     500
#endif

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

static tel_observer_fn     observer;
static tel_log_changed_fn  log_observer;

/* Call log: append-only ring, newest at the end. */
static struct call_log_entry log_buf[CALLLOG_MAX];
static int                   log_n;

/* Index of the most recent entry logged with an unknown number (reject/miss
 * before CLIP arrived) — patched in place if the real number turns up later,
 * even after the call has ended. Any further calllog_add() invalidates it,
 * since positions can shift (dedup-shift or eviction). */
static int pending_fix_idx = -1;

#ifdef GUI_MONKEY
static uint32_t        next_incoming_at;          /* lv_tick to inject next ring */
static int             mock_on = 1;               /* auto-inject incoming calls */
static const char *mock_callers[] = { "0868 000 000" };
static int         mock_idx;
#endif

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

/* Upsert: one entry per number, newest at the end (displayed newest-first).
 * If the number exists, remove the old entry before appending the new one
 * so the list doesn't grow with duplicates and positions stay stable. */
static void calllog_add(const char *number, const char *name,
			enum call_type type, unsigned int dur_sec)
{
	/* Any mutation below shifts positions — a previously tracked pending
	 * index would point at the wrong entry, so drop it unconditionally.
	 * Re-armed below if this new entry itself has no number yet. */
	pending_fix_idx = -1;

	/* Remove existing entry for this number (shift left to fill the gap). */
	for (int i = 0; i < log_n; i++) {
		if (strncmp(log_buf[i].number, number, CALL_NUM_MAX) == 0) {
			for (int j = i; j < log_n - 1; j++)
				log_buf[j] = log_buf[j + 1];
			log_n--;
			break;
		}
	}
	if (log_n == CALLLOG_MAX) {
		for (int i = 0; i < CALLLOG_MAX - 1; i++)
			log_buf[i] = log_buf[i + 1];
		log_n--;
	}
	struct call_log_entry *e = &log_buf[log_n++];
	copy_str(e->number, number, CALL_NUM_MAX);
	copy_str(e->name, name, CALL_NAME_MAX);
	e->type    = (unsigned char)type;
	e->dur_sec = dur_sec;
	if (!number[0]) {
		pending_fix_idx = log_n - 1;
	}
	calllog_save();
}

/* Backfill the number into the still-pending call-log entry (reject/miss
 * whose number wasn't known yet), even though the call has already ended.
 * No-op if the log has moved on (another call logged since) or the number
 * is still unknown. */
static void calllog_patch_pending(const char *number)
{
	if (pending_fix_idx < 0 || pending_fix_idx >= log_n || !number[0]) {
		return;
	}
	struct call_log_entry *e = &log_buf[pending_fix_idx];
	const struct contact  *c = contacts_find_by_phone(number);
	copy_str(e->number, number, CALL_NUM_MAX);
	copy_str(e->name, c ? c->name : "", CALL_NAME_MAX);
	pending_fix_idx = -1;
	calllog_save();
	if (log_observer) {
		log_observer();
	}
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
#ifdef GUI_MONKEY
	next_incoming_at = lv_tick_get() + INCOMING_EVERY_MS;
#endif
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

#ifdef GUI_MONKEY
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
		if ((int32_t)(now - next_incoming_at) >= 0)
			inject_incoming();
		break;
	case TEL_ACTIVE:
		if (now - active_since >= ACTIVE_HANGUP_MS) {
			gui_log("telephony: remote hung up\n");
			telephony_hangup();
		}
		break;
	default:
		break;
	}
}
#endif /* GUI_MONKEY */

void telephony_init(void)
{
	calllog_load();
	cur_info.name   = NULL;
	cur_info.number = cur_number;
#ifdef GUI_MONKEY
	next_incoming_at = lv_tick_get() + INCOMING_EVERY_MS;
	lv_timer_create(telephony_tick, TICK_PERIOD_MS, NULL);
#endif
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
void telephony_set_log_observer(tel_log_changed_fn cb) { log_observer = cb; }

#ifdef GUI_MONKEY
void telephony_set_mock(int on) { mock_on = on; }
#else
void telephony_set_mock(int on) { (void)on; }
#endif

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

/* Fill in the caller's number once it becomes known after the incoming-call
 * screen already showed blank (CLIP arrived after our wait window). No state
 * transition — just backfills cur_number/name and re-renders. */
void telephony_update_caller_id(const char *number)
{
    if (!number || !number[0])
        return;
    if (state == TEL_INCOMING || state == TEL_ACTIVE) {
        if (cur_number[0])
            return;
        copy_str(cur_number, number, CALL_NUM_MAX);
        resolve_name(number);
        cur_info.name   = cur_name[0] ? cur_name : NULL;
        cur_info.number = cur_number;
        gui_logf("telephony: caller id resolved late: %s\n", cur_name[0] ? cur_name : cur_number);
        if (observer) observer(state);
        return;
    }
    /* Call already ended (rejected/missed before CLIP arrived) — patch the
     * call-log entry directly instead of the live call state. */
    calllog_patch_pending(number);
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
    if (log_observer) log_observer();
}

#ifdef GUI_MONKEY
int telephony_mock_on(void) { return mock_on; }
void telephony_tick_from_client(void) { telephony_tick(NULL); }
#else
int telephony_mock_on(void) { return 0; }
void telephony_tick_from_client(void) {}
#endif

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
