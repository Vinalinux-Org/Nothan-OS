/*
 * services/clear.c - Wipe all persisted data (contacts, SMS, call log)
 *
 * Call services_clear_all() ONCE before porting to a new screen/display
 * so every store starts fresh.  Each service writes an empty blob to its
 * .BIN file on the SD card, so the empty state survives a reboot.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "contacts.h"
#include "messages.h"
#include "telephony.h"

void services_clear_all(void)
{
	contacts_clear();
	sms_clear();
	telephony_calllog_clear();
}
