#ifndef __GUI_SERVICES_CLEAR_H
#define __GUI_SERVICES_CLEAR_H

/*
 * Wipe all persisted data (contacts, SMS, call log).
 *
 * Each service writes an empty blob to its SD-card file, so the next
 * boot also starts fresh.  Safe to call at any time — does not touch
 * the active LVGL screen or state machine.
 */
void services_clear_all(void);

#endif
