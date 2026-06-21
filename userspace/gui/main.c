/*
 * gui/main.c - NothanOS GUI demo
 *
 * Boots into the MiNuong splash, then runs a guided tour through the
 * navigation stack: home launcher -> Contacts list -> a contact's detail
 * -> back -> add-contact form -> home, looping. There is no input device
 * yet (HDMI output), so the tour drives nav_push/nav_pop on a timer in
 * place of taps; the same calls fire from the UI once input lands.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "core/nav.h"
#include "core/log.h"
#include "screens/boot.h"
#include "screens/home.h"
#include "screens/contacts_list.h"
#include "screens/contact_detail.h"
#include "screens/contacts_add.h"
#include "screens/sms_list.h"
#include "screens/sms_chat.h"
#include "screens/dialer.h"
#include "screens/active_call.h"
#include "screens/incoming_call.h"
#include "services/contacts.h"
#include "services/messages.h"
#include "services/telephony.h"
#include "../lib/syscall.h"

/* Mock caller for the Phone leg of the tour (a known contact). */
static const struct call_info demo_call = { "Nam Hoang", "0988 333 222" };

enum demo_phase {
	PHASE_BOOT,
	PHASE_HOME,		/* home settled, about to sweep the grid */
	PHASE_HOME_SCROLL_DOWN,
	PHASE_HOME_SCROLL_UP,
	PHASE_LIST,		/* Contacts list shown */
	PHASE_DETAIL,		/* a contact's detail shown */
	PHASE_LIST_RETURN,	/* popped back to the list */
	PHASE_ADD,		/* add-contact form shown */
	PHASE_SMS_HOME,		/* back on home, about to open Messages */
	PHASE_SMS_LIST,		/* SMS conversation list shown */
	PHASE_SMS_CHAT,		/* a conversation thread shown */
	PHASE_PHONE_HOME,	/* back on home, about to open Phone */
	PHASE_DIALER,		/* dial pad shown */
	PHASE_ACTIVE,		/* active-call screen shown */
	PHASE_DIAL_BACK,	/* call ended, back on the dialer */
	PHASE_INCOMING,		/* incoming-call overlay shown */
};

#define BOOT_MS   7000
#define SWEEP_MS  1800		/* full top<->bottom grid scroll */
#define DWELL_MS  1000		/* pause before the next step */

/* Full guided tour (set to 1 to loop only the home scroll for debugging). */
#define DEMO_STATIC  0

static enum demo_phase phase   = PHASE_BOOT;
static unsigned long   phase_t = 0;

/*
 * Periodically dump LVGL heap stats over UART. There is no other way to
 * see memory pressure on hardware (no JTAG). A steadily climbing used%
 * across tour cycles means a leak; a high plateau means the pool is
 * merely small for the peak. max_used is the high-water mark.
 */
static unsigned long mem_log_t = 0;
#define MEM_LOG_MS  2000

static void mem_log(unsigned long now)
{
	if (now - mem_log_t < MEM_LOG_MS)
		return;
	mem_log_t = now;

	lv_mem_monitor_t mon;
	lv_mem_monitor(&mon);

	gui_logf("mem used=%u%% free=%uB biggest=%uB frag=%u%% peak=%uB\n",
		 (unsigned)mon.used_pct, (unsigned)mon.free_size,
		 (unsigned)mon.free_biggest_size, (unsigned)mon.frag_pct,
		 (unsigned)mon.max_used);
}

static void enter_home(void)
{
	gui_log("tour: boot done -> home\n");
	nav_set_root(home_create, NULL);
	nav_show_chrome(true);
}

static void demo_tick(unsigned long now)
{
	unsigned long dt = now - phase_t;

	switch (phase) {
	case PHASE_BOOT:
		if (dt >= BOOT_MS) {
			enter_home();
			phase = PHASE_HOME;
			phase_t = now;
		}
		break;
	case PHASE_HOME:
		if (dt >= DWELL_MS) {
			gui_log("tour: scroll grid down\n");
			home_scroll_to_end(1, SWEEP_MS);
			phase = PHASE_HOME_SCROLL_DOWN;
			phase_t = now;
		}
		break;
	case PHASE_HOME_SCROLL_DOWN:
		if (dt >= SWEEP_MS + DWELL_MS) {
			gui_log("tour: scroll grid up\n");
			home_scroll_to_end(0, SWEEP_MS);
			phase = PHASE_HOME_SCROLL_UP;
			phase_t = now;
		}
		break;
	case PHASE_HOME_SCROLL_UP:
		if (dt >= SWEEP_MS + DWELL_MS) {
#if DEMO_STATIC
			/* Isolation: loop the home scroll forever (keeps the
			 * renderer + shadow/layer allocs busy) but never open
			 * an app — no screen create/delete churn. */
			phase = PHASE_HOME;
#else
			gui_log("tour: open Contacts app\n");
			nav_push(contacts_list_create, NULL);
			phase = PHASE_LIST;
#endif
			phase_t = now;
		}
		break;
#if !DEMO_STATIC
	case PHASE_LIST:
		if (dt >= DWELL_MS + 600) {
			gui_log("tour: open first contact\n");
			nav_push(contact_detail_create,
				 (void *)contacts_get(0));
			phase = PHASE_DETAIL;
			phase_t = now;
		}
		break;
	case PHASE_DETAIL:
		if (dt >= SWEEP_MS) {
			gui_log("tour: back from detail\n");
			nav_pop();		/* back to the list */
			phase = PHASE_LIST_RETURN;
			phase_t = now;
		}
		break;
	case PHASE_LIST_RETURN:
		if (dt >= DWELL_MS) {
			gui_log("tour: open Add contact\n");
			nav_push(contacts_add_create, NULL);
			phase = PHASE_ADD;
			phase_t = now;
		}
		break;
	case PHASE_ADD:
		if (dt >= SWEEP_MS) {
			gui_log("tour: back to home\n");
			nav_to_root();		/* collapse back to home */
			phase = PHASE_SMS_HOME;
			phase_t = now;
		}
		break;
	case PHASE_SMS_HOME:
		if (dt >= DWELL_MS) {
			gui_log("tour: open Messages app\n");
			nav_push(sms_list_create, NULL);
			phase = PHASE_SMS_LIST;
			phase_t = now;
		}
		break;
	case PHASE_SMS_LIST:
		if (dt >= DWELL_MS + 600) {
			gui_log("tour: open first chat\n");
			nav_push(sms_chat_create,
				 (void *)sms_conversation_get(0));
			phase = PHASE_SMS_CHAT;
			phase_t = now;
		}
		break;
	case PHASE_SMS_CHAT:
		if (dt >= SWEEP_MS) {
			gui_log("tour: back to home\n");
			nav_to_root();		/* collapse back to home */
			phase = PHASE_PHONE_HOME;
			phase_t = now;
		}
		break;
	case PHASE_PHONE_HOME:
		if (dt >= DWELL_MS) {
			gui_log("tour: open Phone app\n");
			nav_push(dialer_create, NULL);
			phase = PHASE_DIALER;
			phase_t = now;
		}
		break;
	case PHASE_DIALER:
		if (dt >= DWELL_MS + 600) {
			gui_log("tour: place call\n");
			nav_push(active_call_create, (void *)&demo_call);
			phase = PHASE_ACTIVE;
			phase_t = now;
		}
		break;
	case PHASE_ACTIVE:
		if (dt >= SWEEP_MS) {
			gui_log("tour: end call\n");
			nav_pop();		/* back to the dialer */
			phase = PHASE_DIAL_BACK;
			phase_t = now;
		}
		break;
	case PHASE_DIAL_BACK:
		if (dt >= DWELL_MS) {
			gui_log("tour: incoming call\n");
			nav_push(incoming_call_create, (void *)&demo_call);
			phase = PHASE_INCOMING;
			phase_t = now;
		}
		break;
	case PHASE_INCOMING:
		if (dt >= SWEEP_MS) {
			gui_log("tour: return to home (loop)\n");
			nav_to_root();		/* collapse back to home */
			phase = PHASE_HOME;	/* loop the tour */
			phase_t = now;
		}
		break;
#endif /* !DEMO_STATIC */
	}
}

void main(void)
{
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();

	nav_init();
	nav_set_root(boot_create, NULL);
	gui_log("ready\n");

	unsigned long last_tick = getticks();
	phase_t = last_tick;

	while (1) {
		unsigned long now = getticks();
		lv_tick_inc((uint32_t)(now - last_tick));
		last_tick = now;

		demo_tick(now);
		mem_log(now);
		lv_task_handler();
		yield();
	}
}
