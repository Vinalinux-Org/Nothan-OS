/*
 * phone_daemon/sms_rx.c - incoming multipart-SMS reassembly (see sms_rx.h)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "sms_rx.h"
#include "../lib/string.h"   /* strlen, strcmp, strncpy, memcpy — inline, host-safe */

#define RX_SENDERS     4      /* senders buffered concurrently                */
/* Cap the reassembled text so the JSON frame to the GUI (PHONE_JSON_MAX=2048,
 * text may double under escaping) always closes validly, and to match the
 * GUI's own SMS_TEXT_MAX=1024 — beyond this the GUI truncates anyway. */
#define RX_TEXT_CAP    1024
#define RX_ADDR_MAX    40     /* sender address buffer                        */
#define RX_TS_MAX      24     /* timestamp buffer                            */

/* Max user chars in a *middle* concatenated part. A middle part is packed to
 * exactly this; the final part (and a standalone) is shorter. */
#define UCS2_PART_FULL 67
#define GSM7_PART_FULL 153

/* Safety net only: a buffered sender with no short tail is flushed after this
 * long. Reassembly correctness comes from the length rule, not this timer, so
 * it can be generous. */
#define RX_COLLECT_MS  5000

typedef struct {
	int           active;
	char          sender[RX_ADDR_MAX];
	char          ts[RX_TS_MAX];
	char          text[RX_TEXT_CAP];
	int           len;
	unsigned long expire_ms;
} rx_slot;

static rx_slot        g_slot[RX_SENDERS];
static sms_rx_emit_fn g_emit;

void sms_rx_init(sms_rx_emit_fn emit)
{
	g_emit = emit;
	for (int i = 0; i < RX_SENDERS; i++) {
		g_slot[i].active = 0;
		g_slot[i].len    = 0;
	}
}

static void slot_reset(rx_slot *s)
{
	s->active  = 0;
	s->len     = 0;
	s->text[0] = '\0';
}

static void slot_open(rx_slot *s, const char *sender, const char *ts)
{
	s->active  = 1;
	s->len     = 0;
	s->text[0] = '\0';
	strncpy(s->sender, sender ? sender : "", sizeof(s->sender) - 1);
	s->sender[sizeof(s->sender) - 1] = '\0';
	strncpy(s->ts, ts ? ts : "", sizeof(s->ts) - 1);
	s->ts[sizeof(s->ts) - 1] = '\0';
}

/* Emit whatever is buffered (if anything) and free the slot. */
static void slot_emit(rx_slot *s)
{
	if (s->active && s->len > 0 && g_emit) {
		g_emit(s->sender, s->text, s->ts);
	}
	slot_reset(s);
}

static rx_slot *slot_find(const char *sender)
{
	for (int i = 0; i < RX_SENDERS; i++) {
		if (g_slot[i].active &&
		    strcmp(g_slot[i].sender, sender) == 0) {
			return &g_slot[i];
		}
	}
	return 0;
}

/* Get a slot for `sender`: a free one, or evict the oldest (emitting what it
 * held first, so a stalled message is never silently dropped). */
static rx_slot *slot_acquire(const char *sender, const char *ts)
{
	rx_slot *oldest = 0;
	for (int i = 0; i < RX_SENDERS; i++) {
		if (!g_slot[i].active) {
			slot_open(&g_slot[i], sender, ts);
			return &g_slot[i];
		}
		if (!oldest || g_slot[i].expire_ms < oldest->expire_ms) {
			oldest = &g_slot[i];
		}
	}
	slot_emit(oldest);               /* flush what we had, then reuse */
	slot_open(oldest, sender, ts);
	return oldest;
}

static void slot_append(rx_slot *s, const char *text)
{
	int t = (int)strlen(text);
	if (s->len + t >= RX_TEXT_CAP - 1) {
		t = RX_TEXT_CAP - 1 - s->len;
	}
	if (t <= 0) {
		return;
	}
	memcpy(s->text + s->len, text, (size_t)t);
	s->len += t;
	s->text[s->len] = '\0';
}

static int part_is_full(int part_chars, int is_ucs2)
{
	/* A middle concatenated part is packed to exactly the max. UCS-2 code
	 * units are counted exactly (hexlen/4); GSM-7 septet counts can shrink
	 * when the text uses 2-septet extended chars, so accept "near full". */
	if (is_ucs2) {
		return part_chars == UCS2_PART_FULL;
	}
	return part_chars >= GSM7_PART_FULL - 4;
}

void sms_rx_part(const char *sender, const char *ts, const char *text,
		 int part_chars, int is_ucs2, unsigned long now_ms)
{
	if (!sender) sender = "";
	if (!text)   text   = "";

	rx_slot *s = slot_find(sender);
	if (!s) {
		s = slot_acquire(sender, ts);
	}
	slot_append(s, text);

	if (part_is_full(part_chars, is_ucs2)) {
		/* More parts likely follow — keep the buffer open, arm the net. */
		s->expire_ms = now_ms + RX_COLLECT_MS;
	} else {
		/* Short part → final (or standalone). Emit the whole message. */
		slot_emit(s);
	}
}

void sms_rx_tick(unsigned long now_ms)
{
	for (int i = 0; i < RX_SENDERS; i++) {
		if (g_slot[i].active && now_ms >= g_slot[i].expire_ms) {
			slot_emit(&g_slot[i]);
		}
	}
}
