/*
 * services/telephony.c - Call actions (mock)
 *
 * Logging stubs for now. Replace with SIM7600CE AT commands over UART
 * (ATD/ATA/ATH) without touching the Phone screens.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "telephony.h"
#include "../core/log.h"

void telephony_dial(const char *number)
{
	gui_logf("telephony: dial %s\n", number && number[0] ? number : "(empty)");
}

void telephony_answer(const struct call_info *call)
{
	const char *who = "?";
	if (call)
		who = call->name ? call->name : call->number;
	gui_logf("telephony: answer %s\n", who);
}

void telephony_hangup(void)
{
	gui_log("telephony: hangup\n");
}

void telephony_mute(int on)
{
	gui_logf("telephony: mute %s\n", on ? "on" : "off");
}
