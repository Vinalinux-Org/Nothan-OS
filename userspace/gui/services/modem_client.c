/*
 * gui/services/modem_client.c - IPC client matching phone_daemon's protocol
 *
 * Mở /dev/phone_fe, gửi HELLO, pump mỗi tick: đọc byte → ráp frame
 * (dùng phone_frame.c) → json_parse → dispatch event gọi vào telephony/messages.
 *
 * Gửi lệnh (cmd_*) thì encode frame + write xuống daemon.
 *
 * Dùng lại phone_frame.c/json.c từ phone_daemon/ (include theo relative path).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include "modem_client.h"
#include "contacts.h"
#include "telephony.h"
#include "../core/log.h"

#include "../../phone_daemon/phone_frame.h"
#include "../../phone_daemon/json.h"
#include "../../lib/syscall.h"
#include "../../lib/string.h"
#include "../../lib/printf.h"

/* Forward declarations of the seam entry points we add to telephony/messages.
 * These are defined in telephony.c and messages.c respectively. */
extern void telephony_on_incoming(const char *number);
extern void telephony_on_connected(void);
extern void telephony_on_remote_end(int is_missed);
extern void telephony_log_missed_direct(const char *number);
extern void sms_on_received(const char *peer, const char *text);
extern void messages_set_mock(int on);

/* Forward: called back from the shell so the mock timer keeps working for
 * MONKEY builds (where there is no phone_daemon). */
extern int  telephony_mock_on(void);
extern void telephony_tick_from_client(void);

/* IPC state */
static int  phone_fd = -1;
static int  be_seq  = 0;   /* last-known seq from daemon, for ACK/dedup */
static int  fe_boot = 0;

/* Network status (updated by NET_REG / SIGNAL events) */
static int  g_net_reg = 0;   /* 1 = registered (home or roaming) */
static int  g_rssi    = 0;   /* last rssi from SIGNAL event */

static modem_signal_cb_t g_signal_cb;

int modem_net_registered(void) { return phone_fd < 0 ? 1 : g_net_reg; }
void modem_set_signal_cb(modem_signal_cb_t cb) { g_signal_cb = cb; }

/* ─── non-blocking frame assembler (mirrors the daemon's fe_parse) ─── */

/* Forward: event dispatch (defined after this section). */
static void dispatch_event(const char *json);

static unsigned char fe_buf[PHONE_FRAME_MAX * 2];
static int           fe_buf_len = 0;

/* Drain parsed frames from fe_buf. */
static void fe_parse(void)
{
    for (;;) {
        int i, magic = -1, len;
        /* Scan for magic markers */
        for (i = 0; i + 1 < fe_buf_len; i++)
            if (fe_buf[i] == PHONE_MAGIC0 && fe_buf[i + 1] == PHONE_MAGIC1) {
                magic = i; break;
            }
        if (magic < 0) {
            if (fe_buf_len > 0) { fe_buf[0] = fe_buf[fe_buf_len - 1]; fe_buf_len = 1; }
            return;
        }
        if (magic > 0) {
            memmove(fe_buf, fe_buf + magic, fe_buf_len - magic);
            fe_buf_len -= magic;
        }
        if (fe_buf_len < 4) return;
        len = fe_buf[2] | (fe_buf[3] << 8);
        if (len == 0 || len >= PHONE_JSON_MAX) {
            memmove(fe_buf, fe_buf + 2, fe_buf_len - 2);
            fe_buf_len -= 2; continue;
        }
        if (fe_buf_len < len + 6) return;
        {
            uint16_t rx_crc = fe_buf[4 + len] | (fe_buf[5 + len] << 8);
            uint16_t calc   = phone_crc16(fe_buf + 4, (size_t)len);
            char json[PHONE_JSON_MAX];
            if (rx_crc == calc) {
                memcpy(json, fe_buf + 4, (size_t)len);
                json[len] = '\0';
                /* Dispatch */
                dispatch_event(json);
            }
            memmove(fe_buf, fe_buf + len + 6, fe_buf_len - (len + 6));
            fe_buf_len -= (len + 6);
        }
    }
}

/* ─── frame writers ─── */

static void fe_raw_write(const char *json)
{
    uint8_t frame[PHONE_FRAME_MAX];
    int total = phone_frame_encode(frame, sizeof(frame), json, strlen(json));
    if (total < 0) return;
    writefile(phone_fd, (const char *)frame, (unsigned long)total);
}


/* Format a phone number for display: "+84 70 2507253" style.
 * Input is the raw JSON string (e.g. "+84702507253").
 * Output in `out` is the formatted number. */
/* Format phone number for display (calls + SMS, all local):
 * +84702507253 → 098 8333222, 0988333222 → 098 8333222.
 * Strip non-digits, convert 84→0, space after first 3 digits. */
static void fmt_call(char *out, int sz, const char *raw)
{
	int i = 0, p; char t[32] = {0};
	while (*raw && i < 31) { if (*raw>='0'&&*raw<='9') t[i++]=*raw; raw++; }
	t[i] = '\0';
	if (t[0]=='8' && t[1]=='4') {
		t[0] = '0';
		for (p = 1; p < i; p++) t[p] = t[p+1];
	}
	/* space after the first 3 digits */
	for (i = 0, p = 0; t[i] && p < sz-1; i++) {
		if (i == 3 && p < sz-1) out[p++] = ' ';
		out[p++] = t[i];
	}
	out[p] = '\0';
}

/* ─── event dispatch ─── */

static void send_ack(int seq)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"type\":\"ACK\",\"seq\":%d}", seq);
    fe_raw_write(buf);
}

static void dispatch_event(const char *json)
{
    char type[32]; int seq = -1;
    if (json_get_str(json, "type", type, sizeof(type)) < 0) return;

    /* ACK — we send ACKs; we never get ACKed (daemon sends events, we ack). */
    if (strcmp(type, "ACK") == 0) return;

    /* READY beacon — carries the daemon's CURRENT reliable-seq counter just
     * to report it, not as a numbered message of its own. It reuses whatever
     * seq the last reliable send used, so it must bypass the dedup below —
     * otherwise it collides with that message's seq and gets silently
     * dropped, along with "callnum" (late-arriving caller ID backfill). */
    if (strcmp(type, "READY") == 0) {
        char callnum[32];
        callnum[0] = '\0';
        json_get_str(json, "callnum", callnum, sizeof(callnum));
        if (callnum[0]) telephony_update_caller_id(callnum);
        return;
    }

    /* Extract seq for dedup + ACK */
    json_get_int(json, "seq", &seq);

    /* Dedup (same seq from daemon = retransmit) */
    if (seq > 0) {
        if (seq <= be_seq) return;  /* already processed */
        be_seq = seq;
    }

    /* ── Call events ── */
    if (strcmp(type, "CALL_IN") == 0) {
        char raw[32];
        json_get_str(json, "num", raw, sizeof(raw));
        if (seq > 0) send_ack(seq);
        telephony_on_incoming(raw);
        return;
    }
    if (strcmp(type, "CALL_ACT") == 0) {
        if (seq > 0) send_ack(seq);
        telephony_on_connected();
        return;
    }
    if (strcmp(type, "CALL_END") == 0) {
        if (seq > 0) send_ack(seq);
        telephony_on_remote_end(0);
        return;
    }
    if (strcmp(type, "CALL_MISS") == 0) {
        if (seq > 0) send_ack(seq);
        /* CCWA case: daemon sends CALL_MISS(num) after CALL_END has already moved
         * state to IDLE. Extract num — if present and state is idle, log directly. */
        {
            char missed_num[32];
            json_get_str(json, "num", missed_num, sizeof(missed_num));
            if (missed_num[0] && telephony_state() == TEL_IDLE)
                telephony_log_missed_direct(missed_num);
            else
                telephony_on_remote_end(1);
        }
        return;
    }
    /* ── SMS events ── */
    if (strcmp(type, "SMS_IN") == 0) {
        char from[32], text[1024];
        from[0] = text[0] = '\0';
        json_get_str(json, "from", from, sizeof(from));
        fmt_call(from, sizeof(from), from);
        json_get_str(json, "text", text, sizeof(text));
        if (seq > 0) send_ack(seq);
        sms_on_received(from, text);
        return;
    }
    if (strcmp(type, "SMS_ACK") == 0) {
        int ref = -1;
        json_get_int(json, "ref", &ref);
        gui_logf("modem_client: SMS sent ok ref=%d\n", ref);
        return;
    }
    if (strcmp(type, "SMS_ERR") == 0) {
        int code = -1; char msg[64];
        json_get_int(json, "code", &code);
        msg[0] = '\0';
        json_get_str(json, "msg", msg, sizeof(msg));
        gui_logf("modem_client: SMS send error %d: %s\n", code, msg);
        return;
    }
    /* Signal strength */
    if (strcmp(type, "SIGNAL") == 0) {
        int rssi = 0;
        json_get_int(json, "rssi", &rssi);
        g_rssi = rssi;
        gui_logf("modem_client: signal rssi=%d\n", g_rssi);
        if (g_signal_cb) g_signal_cb(g_net_reg, g_rssi);
        return;
    }
    /* Network registration */
    if (strcmp(type, "NET_REG") == 0) {
        int stat = 0;
        json_get_int(json, "stat", &stat);
        g_net_reg = (stat == 1 || stat == 5);
        gui_logf("modem_client: net_reg stat=%d registered=%d\n", stat, g_net_reg);
        if (g_signal_cb) g_signal_cb(g_net_reg, g_rssi);
        return;
    }
    /* SIM state */
    if (strcmp(type, "SIM_STAT") == 0) {
        char state[16];
        if (json_get_str(json, "state", state, sizeof(state)) >= 0)
            gui_logf("modem_client: SIM state = %s\n", state);
        return;
    }
    if (strcmp(type, "MODEM_DOWN") == 0) {
        if (seq > 0) send_ack(seq);
        g_net_reg = 0;
        if (g_signal_cb) g_signal_cb(0, 0);
        gui_log("modem_client: modem down\n");
        return;
    }
    if (strcmp(type, "MODEM_UP") == 0) {
        if (seq > 0) send_ack(seq);
        gui_log("modem_client: modem up\n");
        return;
    }
    if (strcmp(type, "PHONE_NUM") == 0) {
        char num[32];
        num[0] = '\0';
        json_get_str(json, "num", num, sizeof(num));
        gui_logf("modem_client: own number %s\n", num);
        return;
    }
}

/* ─── public API ─── */

void modem_client_init(void)
{
    phone_fd = open("/dev/phone_fe", 0);  /* O_RDWR not needed for char device */
    if (phone_fd < 0) {
        gui_log("modem_client: no /dev/phone_fe — running with mock\n");
        return;
    }
    /* Real daemon connected — disable mock injectors so fake SMS/calls
     * don't mix with real modem events. */
    telephony_set_mock(0);
    messages_set_mock(0);
    /* Send HELLO handshake */
    fe_boot = (int)getticks();  /* best-effort boot id */
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"type\":\"CMD_HELLO\",\"fe_boot\":%d,\"last_seq\":0}", fe_boot);
    fe_raw_write(buf);
}

void modem_pump(void)
{
    if (phone_fd < 0) return;
    for (;;) {
        if (fe_buf_len >= (int)sizeof(fe_buf)) fe_buf_len = 0;
        long n = read(phone_fd, fe_buf + fe_buf_len,
                      sizeof(fe_buf) - (size_t)fe_buf_len);
        if (n > 0) {
            fe_buf_len += (int)n;
            fe_parse();
            if (n < (long)(sizeof(fe_buf) - (size_t)fe_buf_len)) break;
            continue;
        }
        break;  /* no data */
    }
}

/* ─── command helpers ─── */

static void send_json(const char *json)
{
    if (phone_fd < 0) return;
    fe_raw_write(json);
}

void modem_cmd_dial(const char *number)
{
    char json[PHONE_JSON_MAX];
    json_builder_t b;
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", "CMD_DIAL");
    json_str(&b, "num", number ? number : "");
    json_end(&b);
    send_json(json);
}

void modem_cmd_answer(void)
{
    send_json("{\"type\":\"CMD_ANSWER\"}");
}
void modem_cmd_hangup(void)
{
    send_json("{\"type\":\"CMD_HANGUP\"}");
}
void modem_cmd_reject(void)
{
    send_json("{\"type\":\"CMD_REJECT\"}");
}
void modem_cmd_mute(int on)
{
    send_json(on ? "{\"type\":\"CMD_MUTE\"}" : "{\"type\":\"CMD_UNMUTE\"}");
}

void modem_cmd_sms_send(const char *peer, const char *text)
{
    /* Resolve the display name to a phone number so the daemon sends a
     * valid AT+CMGS.  If the peer is not a saved contact it is treated
     * as a raw number (e.g. "0903..."). */
    const struct contact *c = contacts_find_by_name(peer);
    const char *num = c ? c->phone : (peer ? peer : "");

    char json[PHONE_JSON_MAX];
    json_builder_t b;
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", "CMD_SMS");
    json_str(&b, "num", num);
    json_str(&b, "text", text ? text : "");
    json_end(&b);
    send_json(json);
}
