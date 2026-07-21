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
#ifdef GUI_MONKEY
#define RECV_EVERY_MS   40000
#define TICK_PERIOD_MS  1000
#endif

/* Whole store, kept in BSS (too big for the stack) and written verbatim. */
static struct {
	unsigned int            magic;
	int                     count;
	struct sms_conversation conv[SMS_CONV_MAX];
} store;

#ifdef GUI_MONKEY
static const char *mock_texts[] = {
	"On my way",
	"Call me when you can",
	"Thanks!",
	"See you soon",
};
static int      mock_text_idx;
static uint32_t next_recv_at;
static int      recv_on = 1;   /* auto-inject inbound SMS */
#endif

/* Deferred-save timer: batches FAT writes so the GUI main loop is not
 * blocked on every incoming SMS/read.  Fires once after 300 ms of
 * inactivity, then auto-deletes. */
static lv_timer_t *g_save_timer;

/*
 * Compact on-disk format — only the bytes actually in use, no empty padding.
 * The old code dumped the whole ~515 KB fixed struct on every message, and the
 * polled MMC write (IRQs masked per 512 B block) stalled the whole system for
 * ~1000 blocks. A few short threads serialize to a few KB → the SD write is as
 * cheap as contacts/calllog → no freeze.
 *
 *   [4] magic
 *   [2] conversation count
 *   per conversation:
 *     [1] peer length, [peer] bytes
 *     [1] unread   [1] message count
 *     per message: [1] sent  [2] text length  [text] bytes
 */
#define SMS_BLOB_MAX (32 * 1024)
static unsigned char sms_blob[SMS_BLOB_MAX];

static int slen(const char *s, int max)
{
	int n = 0;
	while (n < max && s[n]) {
		n++;
	}
	return n;
}

static int sms_serialize(void)
{
	int n = 0;
	sms_blob[n++] = (unsigned char)(SMS_MAGIC & 0xFF);
	sms_blob[n++] = (unsigned char)((SMS_MAGIC >> 8) & 0xFF);
	sms_blob[n++] = (unsigned char)((SMS_MAGIC >> 16) & 0xFF);
	sms_blob[n++] = (unsigned char)((SMS_MAGIC >> 24) & 0xFF);
	int count_at = n;
	n += 2;				/* patched after the loop */
	int written = 0;
	for (int c = 0; c < store.count; c++) {
		struct sms_conversation *cv = &store.conv[c];
		int pl = slen(cv->peer, SMS_PEER_MAX);
		int need = 1 + pl + 2;
		for (int m = 0; m < cv->message_count; m++)
			need += 3 + slen(cv->messages[m].text, SMS_TEXT_MAX);
		if (n + need > SMS_BLOB_MAX)
			break;			/* store is newest-first → keep newest */
		sms_blob[n++] = (unsigned char)pl;
		for (int k = 0; k < pl; k++)
			sms_blob[n++] = (unsigned char)cv->peer[k];
		sms_blob[n++] = (unsigned char)(cv->unread > 255 ? 255 : cv->unread);
		sms_blob[n++] = (unsigned char)cv->message_count;
		for (int m = 0; m < cv->message_count; m++) {
			int tl = slen(cv->messages[m].text, SMS_TEXT_MAX);
			sms_blob[n++] = cv->messages[m].sent;
			sms_blob[n++] = (unsigned char)(tl & 0xFF);
			sms_blob[n++] = (unsigned char)((tl >> 8) & 0xFF);
			for (int k = 0; k < tl; k++)
				sms_blob[n++] = (unsigned char)cv->messages[m].text[k];
		}
		written++;
	}
	sms_blob[count_at]     = (unsigned char)(written & 0xFF);
	sms_blob[count_at + 1] = (unsigned char)((written >> 8) & 0xFF);
	return n;
}

/* Rebuild the store from sms_blob[0..len). Returns conv count, or -1 if the
 * blob is missing/short/corrupt (caller then seeds the demo threads). */
static int sms_deserialize(int len)
{
	if (len < 6)
		return -1;
	unsigned int magic = sms_blob[0] | (sms_blob[1] << 8) |
			     (sms_blob[2] << 16) | ((unsigned)sms_blob[3] << 24);
	if (magic != SMS_MAGIC)
		return -1;
	int n = 4;
	int count = sms_blob[n] | (sms_blob[n + 1] << 8);
	n += 2;
	if (count < 0 || count > SMS_CONV_MAX)
		return -1;
	store.count = 0;
	for (int c = 0; c < count; c++) {
		if (n + 1 > len)
			return -1;
		int pl = sms_blob[n++];
		if (pl >= SMS_PEER_MAX || n + pl + 2 > len)
			return -1;
		struct sms_conversation *cv = &store.conv[store.count];
		for (int k = 0; k < pl; k++)
			cv->peer[k] = (char)sms_blob[n++];
		cv->peer[pl] = '\0';
		cv->unread = sms_blob[n++];
		int mc = sms_blob[n++];
		if (mc > SMS_PER_CONV)
			return -1;
		cv->message_count = 0;
		for (int m = 0; m < mc; m++) {
			if (n + 3 > len)
				return -1;
			unsigned char sent = sms_blob[n++];
			int tl = sms_blob[n] | (sms_blob[n + 1] << 8);
			n += 2;
			if (tl >= SMS_TEXT_MAX || n + tl > len)
				return -1;
			struct sms_message *msg = &cv->messages[cv->message_count++];
			for (int k = 0; k < tl; k++)
				msg->text[k] = (char)sms_blob[n++];
			msg->text[tl] = '\0';
			msg->sent = sent;
		}
		store.count++;
	}
	return store.count;
}

static void messages_save(void)
{
	/* Compact write — only real data, so the SD write stays small and does not
	 * stall the system the way the full-struct dump did. Still deferred 300 ms
	 * (schedule_save) so a burst of messages coalesces into one write. */
	storage_write(SMS_PATH, sms_blob, sms_serialize());
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

/* Move a conversation to the front so the thread with the newest activity
 * sorts to the top of the list (like a real phone). The ones above it shift
 * down by one. */
static void move_to_front(int idx)
{
	if (idx <= 0) {
		return;
	}
	struct sms_conversation tmp = store.conv[idx];
	for (int i = idx; i > 0; i--) {
		store.conv[i] = store.conv[i - 1];
	}
	store.conv[0] = tmp;
}

/* Defined in screens/sms_chat.c — keeps an open thread pointing at its own
 * conversation across the move_to_front() shuffle above. */
extern void sms_chat_reindex(int moved_from);

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
	messages_save();		/* persist the empty store */
}

#ifdef GUI_MONKEY
static void messages_tick(lv_timer_t *t);
#endif

void messages_init(void)
{
	/* Load persisted threads from SD. No fake seed anymore — the list simply
	 * starts empty until real SMS arrive (compact format, see messages_save). */
	int len = storage_read(SMS_PATH, sms_blob, SMS_BLOB_MAX);
	if (len < 6 || sms_deserialize(len) < 0)
		store.count = 0;

#ifdef GUI_MONKEY
	next_recv_at = lv_tick_get() + RECV_EVERY_MS;
	lv_timer_create(messages_tick, TICK_PERIOD_MS, NULL);
#endif
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

void sms_conversation_delete(int conv_index)
{
	if (conv_index < 0 || conv_index >= store.count) {
		return;
	}
	gui_logf("sms: delete thread %s\n", store.conv[conv_index].peer);
	/* Shift the tail up over the removed slot, then shrink. Struct copy is
	 * fine — the store already moves whole conversations (move_to_front). */
	for (int i = conv_index; i < store.count - 1; i++) {
		store.conv[i] = store.conv[i + 1];
	}
	store.count--;
	schedule_save();
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
	/* Bump this thread to the top; fix an open chat's index first so it keeps
	 * showing the right conversation after the store shuffles. */
	sms_chat_reindex(idx);
	move_to_front(idx);
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

#ifdef GUI_MONKEY
static void mock_receive(void)
{
	if (store.count == 0)
		return;
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
#else
void messages_set_mock(int on) { (void)on; }
#endif
