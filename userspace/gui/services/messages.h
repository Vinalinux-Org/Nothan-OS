#ifndef __GUI_MESSAGES_H
#define __GUI_MESSAGES_H

/*
 * Messages store — the data layer behind the SMS app.
 *
 * Real, mutable, persisted state (not the old read-only mock): conversations
 * live in fixed-size arrays so they serialize straight to /SMS.BIN, and
 * messages can be sent and received. The hardware boundary is mocked — an
 * lv_timer injects an incoming SMS now and then — so the send/receive UX is
 * complete. When the SIM7600CE lands, sms_send() becomes AT+CMGS and the mock
 * timer becomes +CMTI handling; the screens stay unchanged.
 */

#define SMS_TEXT_MAX   160
#define SMS_PEER_MAX   32
#define SMS_PER_CONV   16
#define SMS_CONV_MAX   10

struct sms_message {
	char          text[SMS_TEXT_MAX];
	unsigned char sent;   /* 1 = sent by us (right bubble), 0 = received */
};

struct sms_conversation {
	char               peer[SMS_PEER_MAX];   /* contact name or raw number */
	struct sms_message messages[SMS_PER_CONV];
	int                message_count;
	int                unread;               /* received-but-unseen count */
};

/* Load persisted threads (else seed a mock) and start the mock receiver. */
void messages_init(void);

/* Enable/disable the mock inbound-SMS injector (on by default). Turn off
 * to isolate pure-GUI testing (no receive, no background FAT write). */
void messages_set_mock(int on);

int  sms_conversation_count(void);
const struct sms_conversation *sms_conversation_get(int index);

/* Find a thread by exact peer, returning its index or -1. */
int  sms_conversation_find(const char *peer);
/* Like find, but creates an empty thread if none exists. Returns index or -1. */
int  sms_conversation_find_or_create(const char *peer);

/* Last message text of a thread (for the list preview), or "". */
const char *sms_preview(const struct sms_conversation *c);

/* Append an outgoing message to a thread and persist. */
void sms_send(int conv_index, const char *text);
/* Clear a thread's unread count and persist. */
void sms_mark_read(int conv_index);
/* Total unread across all threads (for a badge). */
int  sms_total_unread(void);

#endif
