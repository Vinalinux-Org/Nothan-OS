#ifndef __GUI_MESSAGES_H
#define __GUI_MESSAGES_H

/*
 * Messages store — the data layer behind the SMS app.
 *
 * The UI talks only to this interface. Today the backend is an in-RAM
 * mock; when the SIM7600CE telephony stack lands, the SMS send/receive
 * path replaces this file and the screens stay unchanged. Mirrors the
 * shape of the contacts store.
 */

struct sms_message {
	const char *text;
	const char *time;
	int         sent;   /* 1 = sent by us (right bubble), 0 = received */
};

struct sms_conversation {
	const char               *peer;        /* contact name or raw number */
	const char               *preview;     /* last message, one line */
	const char               *time;        /* last activity: "10:42"/"Mon" */
	const char               *date_label;  /* separator inside the thread */
	const struct sms_message *messages;
	int                       message_count;
};

int sms_conversation_count(void);
const struct sms_conversation *sms_conversation_get(int index);
const struct sms_conversation *sms_conversation_find(const char *peer);

#endif
