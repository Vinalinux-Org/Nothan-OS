/*
 * services/messages.c - SMS store (mutable, persisted) + mock receiver
 *
 * Conversations live in a static store (BSS, not the stack — the blob is
 * tens of KB) and persist to /SMS.BIN as one flat write. sms_send() appends
 * an outgoing message; a mock lv_timer injects an incoming one now and then
 * and pokes the active screen so the list/thread refreshes. Swap both for
 * SIM7600CE AT+CMGS / +CMTI later without touching the screens.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "messages.h"
#include "modem_client.h"
#include "telephony.h"
#include "../core/log.h"
#include "storage.h"
#include "lvgl/lvgl.h"

#define SMS_PATH        "/SMS.BIN"
#define SMS_MAGIC       0x534D5332u  /* "SMS1" */
#define RECV_EVERY_MS   40000
#define TICK_PERIOD_MS  1000

/* Whole store, kept in BSS (too big for the stack) and written verbatim. */
static struct {
	unsigned int            magic;
	int                     count;
	struct sms_conversation conv[SMS_CONV_MAX];
} store;

/* Mock inbound traffic: canned texts dropped into rotating peers. */
static const char *mock_texts[] = {
	"On my way",
	"Call me when you can",
	"Thanks!",
	"See you soon",
};
static int      mock_text_idx;
static uint32_t next_recv_at;
static int      recv_on = 1;   /* auto-inject inbound SMS */

/* Deferred-save timer: batches FAT writes so the GUI main loop is not
 * blocked on every incoming SMS/read.  Fires once after 300 ms of
 * inactivity, then auto-deletes. */
static lv_timer_t *g_save_timer;

static void messages_save(void)
{
	store.magic = SMS_MAGIC;
	storage_write(SMS_PATH, &store, sizeof(store));
}

static void deferred_save_cb(lv_timer_t *t)
{
	(void)t;
	g_save_timer = NULL;
	messages_save();
}

static void schedule_save(void)
{
	if (g_save_timer) {
		lv_timer_reset(g_save_timer);   /* restart the 300 ms count */
	} else {
		g_save_timer = lv_timer_create(deferred_save_cb, 300, NULL);
		lv_timer_set_repeat_count(g_save_timer, 1);   /* one-shot */
	}
}

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


/* Append a message to a thread, dropping the oldest when the thread is full. */
static void append_msg(int idx, const char *text, int sent)
{
	struct sms_conversation *c = &store.conv[idx];
	if (c->message_count == SMS_PER_CONV) {
		for (int i = 0; i < SMS_PER_CONV - 1; i++) {
			c->messages[i] = c->messages[i + 1];
		}
		c->message_count--;
	}
	struct sms_message *m = &c->messages[c->message_count++];
	copy_str(m->text, text, SMS_TEXT_MAX);
	m->sent = (unsigned char)(sent ? 1 : 0);
}

static int create_conv(const char *peer)
{
	if (store.count >= SMS_CONV_MAX) {
		return -1;
	}
	int idx = store.count++;
	struct sms_conversation *c = &store.conv[idx];
	copy_str(c->peer, peer, SMS_PEER_MAX);
	c->message_count = 0;
	c->unread = 0;
	return idx;
}

static void seed_mock(void)
{
	store.count = 0;
}

/* Wipe all conversations and persist the empty store so the next boot
 * also starts from scratch.  Call before porting to a new display.
 */
void sms_clear(void)
{
	/* Cancel any deferred save so the timer doesn't overwrite our empty blob. */
	if (g_save_timer) {
		lv_timer_del(g_save_timer);
		g_save_timer = NULL;
	}
	store.count = 0;
	store.magic = SMS_MAGIC;
	storage_write(SMS_PATH, &store, sizeof(store));
}

static void messages_tick(lv_timer_t *t);

void messages_init(void)
{
	int n = storage_read(SMS_PATH, &store, sizeof(store));
	if (n < (int)(sizeof(store.magic) + sizeof(store.count)) ||
	    store.magic != SMS_MAGIC || store.count < 0 ||
	    store.count > SMS_CONV_MAX)
		seed_mock();

	next_recv_at = lv_tick_get() + RECV_EVERY_MS;
	lv_timer_create(messages_tick, TICK_PERIOD_MS, NULL);
}

int sms_conversation_count(void) { return store.count; }

const struct sms_conversation *sms_conversation_get(int index)
{
	if (index < 0 || index >= store.count) {
		return NULL;
	}
	return &store.conv[index];
}

/* Normalise a phone number for comparison: strip spaces/dashes, +84→0. */
static void norm_peer(char *dst, const char *src, int sz)
{
	int i = 0;
	while (*src && i < sz - 1) {
		char c = *src++;
		if (c >= '0' && c <= '9') dst[i++] = c;
	}
	dst[i] = '\0';
	/* +84XXXXXXXXX → 0XXXXXXXXX: digits-only "84..." is 11 chars. */
	if (i == 11 && dst[0] == '8' && dst[1] == '4') {
		dst[0] = '0';
		for (int j = 1; j < i - 1; j++) dst[j] = dst[j + 1];
		dst[--i] = '\0';
	}
}

int sms_conversation_find(const char *peer)
{
	char key[SMS_PEER_MAX];
	if (!peer)
		return -1;
	norm_peer(key, peer, sizeof(key));
	for (int i = 0; i < store.count; i++) {
		char stored[SMS_PEER_MAX];
		norm_peer(stored, store.conv[i].peer, sizeof(stored));
		if (strcmp(key, stored) == 0)
			return i;
	}
	return -1;
}

int sms_conversation_find_or_create(const char *peer)
{
	int idx = sms_conversation_find(peer);
	if (idx >= 0) {
		return idx;
	}
	return create_conv(peer);
}

const char *sms_preview(const struct sms_conversation *c)
{
	static char buf[SMS_TEXT_MAX];
	const char *src; int w = 0, i = 0;
	if (!c || c->message_count == 0) return "";
	src = c->messages[c->message_count - 1].text;
	/* Copy up to 15 words, then append "..." if there were more. */
	while (*src && w < 15) {
		while (*src == ' ') { if (i < (int)sizeof(buf)-1) buf[i++] = *src; src++; }
		while (*src && *src != ' ') { if (i < (int)sizeof(buf)-1) buf[i++] = *src; src++; }
		w++;
	}
	if (*src) { buf[i++] = '.'; buf[i++] = '.'; buf[i++] = '.'; }
	buf[i] = '\0';
	return i > 0 ? buf : "";
}

void sms_send(int conv_index, const char *text)
{
	if (conv_index < 0 || conv_index >= store.count || !text || !text[0]) {
		return;
	}
	gui_logf("sms: send to %s: %s\n", store.conv[conv_index].peer, text);
	append_msg(conv_index, text, 1);
	schedule_save();
	modem_cmd_sms_send(store.conv[conv_index].peer, text);
}


/* Strip Vietnamese diacritics so text renders on Montserrat
 * (which lacks Vietnamese glyphs).  Converts "Hay rồi bạn ơi"
 * → "Hay roi ban oi".  Handles the UTF-8 sequences the daemon
 * produces from UCS2. */
static void strip_vietnamese(char *s)
{
	unsigned char *src = (unsigned char *)s, dst[SMS_TEXT_MAX], *d = dst;
	while (*src) {
		unsigned c = *src;
		if (c < 0x80) { *d++ = (char)c; src++; continue; }
		/* 2-byte UTF-8:  Latin-1 Supplement + Latin Extended-A (d, i, u, o, u). */
			if ((c & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80) {
				unsigned cp = ((c & 0x1F) << 6) | (src[1] & 0x3F);
				if      (cp >= 0x00C0 && cp <= 0x00C6) *d++ = 'A';  /* A-A, AE */
				else if (cp == 0x00C7) *d++ = 'C';
				else if (cp >= 0x00C8 && cp <= 0x00CB) *d++ = 'E';
				else if (cp >= 0x00CC && cp <= 0x00CF) *d++ = 'I';
				else if (cp == 0x00D0) *d++ = 'D';
				else if (cp == 0x00D1) *d++ = 'N';
				else if (cp >= 0x00D2 && cp <= 0x00D6) *d++ = 'O';
				else if (cp == 0x00D8) *d++ = 'O';
				else if (cp >= 0x00D9 && cp <= 0x00DC) *d++ = 'U';
				else if (cp == 0x00DD) *d++ = 'Y';
				else if (cp == 0x00DE) *d++ = 'P';
				else if (cp == 0x0110 || cp == 0x0111) *d++ = 'd';
				else if (cp == 0x0128 || cp == 0x0129) *d++ = 'I';
				else if (cp == 0x0168 || cp == 0x0169) *d++ = 'U';
				else if (cp == 0x01A0 || cp == 0x01A1) *d++ = 'O';
				else if (cp == 0x01AF || cp == 0x01B0) *d++ = 'U';
				else if (cp >= 0x00E0 && cp <= 0x00E6) *d++ = 'a';
				else if (cp == 0x00E7) *d++ = 'c';
				else if (cp >= 0x00E8 && cp <= 0x00EB) *d++ = 'e';
				else if (cp >= 0x00EC && cp <= 0x00EF) *d++ = 'i';
				else if (cp == 0x00F0) *d++ = 'd';
				else if (cp == 0x00F1) *d++ = 'n';
				else if (cp >= 0x00F2 && cp <= 0x00F6) *d++ = 'o';
				else if (cp == 0x00F8) *d++ = 'o';
				else if (cp >= 0x00F9 && cp <= 0x00FC) *d++ = 'u';
				else if (cp == 0x00FD) *d++ = 'y';
				else if (cp == 0x00FE) *d++ = 'p';
				else if (cp == 0x00FF) *d++ = 'y';
				else *d++ = (char)c;
				src += 2;
				continue;
			}
			/* 3-byte UTF-8:  Vietnamese Extension (U+1EA0-1EFF) */
			if ((c & 0xF0) == 0xE0 && (src[1] & 0xC0) == 0x80 && (src[2] & 0xC0) == 0x80) {
				unsigned cp = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
				char repl = 0;
				if      (cp >= 0x1EA0 && cp <= 0x1EB7) repl = 'a';
				else if (cp >= 0x1EB8 && cp <= 0x1EC7) repl = 'e';
				else if (cp >= 0x1EC8 && cp <= 0x1ECB) repl = 'i';
				else if (cp >= 0x1ECC && cp <= 0x1EE3) repl = 'o';
				else if (cp >= 0x1EE4 && cp <= 0x1EF1) repl = 'u';
				else if (cp >= 0x1EF2 && cp <= 0x1EF9) repl = 'y';
				else if (cp == 0x0110 || cp == 0x0111)  repl = 'd';
				else if (cp == 0x0128 || cp == 0x0129)  repl = 'i';
				else if (cp == 0x0168 || cp == 0x0169)  repl = 'u';
				else if (cp == 0x01A0 || cp == 0x01A1)  repl = 'o';
				else if (cp == 0x01AF || cp == 0x01B0)  repl = 'u';
				*d++ = repl ? repl : (char)c;
				src += 3;
				continue;
			}
			/* Anything else -- pass through */
			*d++ = (char)*src++;
	}
	*d = '\0';
	strncpy(s, (const char *)dst, SMS_TEXT_MAX - 1);
	s[SMS_TEXT_MAX - 1] = '\0';
}

/* ─── Modem-client entry point (called from modem_client.c dispatch) ─── */

/* Deferred screen refresh — called from lv_task_handler, not from
 * modem_pump(), so LVGL input processing is not disturbed. */
static void refresh_screen_cb(void *user_data)
{
	(void)user_data;
	if (telephony_state() != TEL_IDLE)
		return;   /* call overlay is active; call_ui fires SCREEN_LOADED on hang-up */
	lv_obj_send_event(lv_screen_active(), LV_EVENT_SCREEN_LOADED, NULL);
}

void sms_on_received(const char *peer, const char *text)
{
	int idx = sms_conversation_find_or_create(peer);
	if (idx < 0)
		return;
	/* Strip Vietnamese accents — Montserrat font lacks those glyphs. */
	char clean[SMS_TEXT_MAX];
	strncpy(clean, text, sizeof(clean));
	clean[sizeof(clean) - 1] = '\0';
	strip_vietnamese(clean);
	append_msg(idx, clean, 0);
	store.conv[idx].unread++;
	schedule_save();
	gui_logf("sms: recv from %s: %s\n", peer, clean);
	lv_async_call(refresh_screen_cb, NULL);
}

void sms_mark_read(int conv_index)
{
	if (conv_index < 0 || conv_index >= store.count) {
		return;
	}
	if (store.conv[conv_index].unread) {
		store.conv[conv_index].unread = 0;
		schedule_save();
	}
}

int sms_total_unread(void)
{
	int n = 0;
	for (int i = 0; i < store.count; i++) {
		n += store.conv[i].unread;
	}
	return n;
}

/* Drop a mock inbound message into an existing thread, mark it unread, and
 * nudge the active screen so the list/thread refreshes if it is showing. */
static void mock_receive(void)
{
	if (store.count == 0) {
		return;
	}
	int idx  = mock_text_idx % store.count;
	const char *text = mock_texts[mock_text_idx % (int)(sizeof(mock_texts) /
							    sizeof(mock_texts[0]))];
	mock_text_idx++;

	append_msg(idx, text, 0);
	store.conv[idx].unread++;
	schedule_save();
	gui_logf("sms: recv from %s: %s\n", store.conv[idx].peer, text);
}

static void messages_tick(lv_timer_t *t)
{
	(void)t;
	if (recv_on && (int32_t)(lv_tick_get() - next_recv_at) >= 0) {
		mock_receive();
		next_recv_at = lv_tick_get() + RECV_EVERY_MS;
	}
}

void messages_set_mock(int on) { recv_on = on; }
