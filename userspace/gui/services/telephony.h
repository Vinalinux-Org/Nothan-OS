#ifndef __GUI_TELEPHONY_H
#define __GUI_TELEPHONY_H

/*
 * Telephony service — the data + actions behind the Phone app.
 *
 * The screens talk only to this interface. Today the actions are logging
 * stubs; when the SIM7600CE modem lands they become AT commands over
 * UART (ATD<n>; dial, ATA answer, ATH hang up). Same shape as the
 * contacts / messages stores.
 */

struct call_info {
	const char *name;     /* resolved contact name, or NULL if unknown */
	const char *number;
};

void telephony_dial(const char *number);
void telephony_answer(const struct call_info *call);
void telephony_hangup(void);
void telephony_mute(int on);

#endif
