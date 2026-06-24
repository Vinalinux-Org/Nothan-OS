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
#include "../core/log.h"
#include "storage.h"
#include "lvgl/lvgl.h"

#define SMS_PATH        "/SMS.BIN"
#define SMS_MAGIC       0x534D5331u  /* "SMS1" */
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

static void copy_str(char *dst, const char *src, int max)
{
	int i = 0;
	if (src)
		for (; src[i] && i < max - 1; i++)
			dst[i] = src[i];
	dst[i] = '\0';
}

static int str_eq(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return *a == *b;
}

static void messages_save(void)
{
	store.magic = SMS_MAGIC;
	storage_write(SMS_PATH, &store, sizeof(store));
}

/* Append a message to a thread, dropping the oldest when the thread is full. */
static void append_msg(int idx, const char *text, int sent)
{
	struct sms_conversation *c = &store.conv[idx];
	if (c->message_count == SMS_PER_CONV) {
		for (int i = 0; i < SMS_PER_CONV - 1; i++)
			c->messages[i] = c->messages[i + 1];
		c->message_count--;
	}
	struct sms_message *m = &c->messages[c->message_count++];
	copy_str(m->text, text, SMS_TEXT_MAX);
	m->sent = (unsigned char)(sent ? 1 : 0);
}

static int create_conv(const char *peer)
{
	if (store.count >= SMS_CONV_MAX)
		return -1;
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
	int a = create_conv("An Nguyen");
	append_msg(a, "Hey, are we still on for tonight?", 0);
	append_msg(a, "Yes, see you at 7", 1);
	append_msg(a, "Great, I'll book a table", 0);

	int b = create_conv("Bao Tran");
	append_msg(b, "Did you get the files?", 0);
	append_msg(b, "Got them, thanks!", 1);

	int o = create_conv("0987 654 321");
	append_msg(o, "Your OTP is 4821. Do not share it.", 0);
	store.conv[o].unread = 1;
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
	if (index < 0 || index >= store.count)
		return NULL;
	return &store.conv[index];
}

int sms_conversation_find(const char *peer)
{
	if (!peer)
		return -1;
	for (int i = 0; i < store.count; i++)
		if (str_eq(store.conv[i].peer, peer))
			return i;
	return -1;
}

int sms_conversation_find_or_create(const char *peer)
{
	int idx = sms_conversation_find(peer);
	if (idx >= 0)
		return idx;
	return create_conv(peer);
}

const char *sms_preview(const struct sms_conversation *c)
{
	if (!c || c->message_count == 0)
		return "";
	return c->messages[c->message_count - 1].text;
}

void sms_send(int conv_index, const char *text)
{
	if (conv_index < 0 || conv_index >= store.count || !text || !text[0])
		return;
	gui_logf("sms: send to %s: %s\n", store.conv[conv_index].peer, text);
	append_msg(conv_index, text, 1);
	messages_save();
}

void sms_mark_read(int conv_index)
{
	if (conv_index < 0 || conv_index >= store.count)
		return;
	if (store.conv[conv_index].unread) {
		store.conv[conv_index].unread = 0;
		messages_save();
	}
}

int sms_total_unread(void)
{
	int n = 0;
	for (int i = 0; i < store.count; i++)
		n += store.conv[i].unread;
	return n;
}

/* Drop a mock inbound message into an existing thread, mark it unread, and
 * nudge the active screen so the list/thread refreshes if it is showing. */
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
	messages_save();
	gui_logf("sms: recv from %s: %s\n", store.conv[idx].peer, text);

	lv_obj_send_event(lv_screen_active(), LV_EVENT_SCREEN_LOADED, NULL);
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
