/*
 * sim/modem_client_sim.c - Sim stub replacing modem_client.c on the host
 *
 * Non-MONKEY_MOCK: pure no-op — modem_pump() does nothing, commands dropped.
 * The lv_timer mock in telephony.c drives all events on its own schedule.
 *
 * MONKEY_MOCK: modem_pump() injects telephony/SMS URCs with a PRNG, called
 * from at_pump() BEFORE lv_task_handler() — the same ordering as the real
 * main loop (modem_pump → lv_task_handler). This reproduces the timing race
 * where a URC arrives mid-render cycle, which the lv_timer path cannot.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "services/modem_client.h"

#ifdef MONKEY_MOCK
#include <stdint.h>

/* Same entry points the real modem_client.c calls after parsing a URC frame. */
extern void telephony_on_incoming(const char *number);
extern void telephony_on_remote_end(int is_missed);
extern void sms_on_received(const char *peer, const char *text);

static uint32_t s_rng = 0xABCD1234;
static uint32_t pump_rng(void)
{
	s_rng ^= s_rng << 13;
	s_rng ^= s_rng >> 17;
	s_rng ^= s_rng << 5;
	return s_rng;
}

static const char *const s_numbers[] = {
	"0868 000 000", "0912 345 678", "0987 654 321"
};
#define NUM_COUNT 3
#endif /* MONKEY_MOCK */

static modem_signal_cb_t s_signal_cb;

void modem_client_init(void)
{
	if (s_signal_cb)
		s_signal_cb(1, 99);
}

void modem_pump(void)
{
#ifdef MONKEY_MOCK
	uint32_t r = pump_rng();
	/* ~0.5%/frame incoming call URC */
	if (r % 200 == 0)
		telephony_on_incoming(s_numbers[(r >> 8) % NUM_COUNT]);
	/* ~0.3%/frame remote hangup/miss URC */
	else if (r % 333 == 1)
		telephony_on_remote_end(1);
	/* ~0.4%/frame incoming SMS URC */
	else if (r % 250 == 2)
		sms_on_received(s_numbers[(r >> 16) % NUM_COUNT], "test");
#endif
}

int modem_net_registered(void) { return 1; }

void modem_set_signal_cb(modem_signal_cb_t cb)
{
	s_signal_cb = cb;
	if (cb) cb(1, 99);
}

void modem_cmd_dial(const char *number)    { (void)number; }
void modem_cmd_answer(void)                {}
void modem_cmd_hangup(void)                {}
void modem_cmd_reject(void)                {}
void modem_cmd_mute(int on)                { (void)on; }
void modem_cmd_sms_send(const char *peer, const char *text)
{
	(void)peer; (void)text;
}
