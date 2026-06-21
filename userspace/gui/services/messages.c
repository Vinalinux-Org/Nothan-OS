/*
 * services/messages.c - In-RAM mock SMS store
 *
 * Sample conversations so the SMS UI has threads to render. Peers reuse
 * the contact names from the contacts store so the two apps feel linked.
 * ASCII text for now (Vietnamese subset font is a later task). Swap this
 * file for the SIM7600CE SMS path without touching any screen.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stddef.h>
#include "messages.h"

static const struct sms_message an_msgs[] = {
	{ "Hey, are we still on for tonight?", "10:38", 0 },
	{ "Yes, see you at 7",                "10:40", 1 },
	{ "Great, I'll book a table",         "10:41", 0 },
	{ "Perfect",                          "10:42", 1 },
};

static const struct sms_message bao_msgs[] = {
	{ "Did you get the files?", "09:12", 0 },
	{ "Got them, thanks!",      "09:15", 1 },
};

static const struct sms_message binh_msgs[] = {
	{ "Meeting moved to 3pm", "Yesterday", 0 },
	{ "Noted",                "Yesterday", 1 },
	{ "Ok",                   "Yesterday", 0 },
};

static const struct sms_message otp_msgs[] = {
	{ "Your OTP is 4821. Do not share it.", "Mon", 0 },
};

static const struct sms_conversation conversations[] = {
	{ "An Nguyen",    "Perfect",           "10:42",     "Today",
	  an_msgs,   4 },
	{ "Bao Tran",     "Got them, thanks!", "09:15",     "Today",
	  bao_msgs,  2 },
	{ "Binh Le",      "Ok",                "Yesterday", "Yesterday",
	  binh_msgs, 3 },
	{ "0987 654 321", "Your OTP is 4821",  "Mon",       "Monday",
	  otp_msgs,  1 },
};

#define CONV_COUNT  (int)(sizeof(conversations) / sizeof(conversations[0]))

int sms_conversation_count(void)
{
	return CONV_COUNT;
}

const struct sms_conversation *sms_conversation_get(int index)
{
	if (index < 0 || index >= CONV_COUNT)
		return NULL;
	return &conversations[index];
}
