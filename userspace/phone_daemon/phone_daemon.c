/*
 * phone_daemon.c — SIM7600CE modem backend for NothanOS
 *
 * Written by Bui Dinh Hien <buihien29112002@gmail.com>
 *
 * Hardware:
 *   /dev/uart1       → SIM7600CE AT command/URC port
 *   /dev/phone_be    → GUI frontend (phonebus loopback, was /dev/uart4→RPi4)
 *
 * Architecture: SINGLE-OWNER, NON-BLOCKING EVENT LOOP
 *   One process owns both channels (no fork, no shared fd). Super loop:
 *     • read SIM URC/response lines  (/dev/uart1)
 *     • read frontend command frames (/dev/phone_be)
 *     • drive AT command/response FSM (queue + per-command timeout)
 *   Non-blocking I/O with yield() — cooperative scheduling on NothanOS.
 *
 * Why single-owner:
 *   • SMS body sent only after real '>' prompt (no sleep race)
 *   • UART error → close+reopen+re-init recovery, never exit
 *   • URCs (RING/+CLIP/+CMT…) dispatched even during in-flight command
 *
 * Protocol (framed JSON over phonebus):
 *   Critical events carry monotonic seq, retransmitted until FE ACKs.
 *   HELLO handshake + periodic READY beacon resync state independently
 *   of startup order.
 *
 * NothanOS bare-metal: compile with -DPD_NO_POLL -DPD_NO_CLOCK -DPD_NO_USLEEP.
 */

#include "pd_port.h"
#include "json.h"
#include "phone_frame.h"

/* errno storage for the port shim (declared extern in pd_port.h). */
int errno;

#ifndef PD_HAVE_POLL
#  ifdef PD_NO_POLL
#    define PD_HAVE_POLL 0
#  else
#    define PD_HAVE_POLL 1
#  endif
#endif

#ifndef time
#  define time(x)  ((long)0)
#endif

/* Compile-time constants                                              */

#define SIM_DEV         "/dev/uart1"
#define RPI_DEV         "/dev/phone_be"   /* phonebus loopback to the GUI (was /dev/uart4 → RPi) */

#define CALLER_NUM_MAX  32
#define SMS_TEXT_MAX    640
#define SMS_HEX_MAX     1024
#define USSD_CODE_MAX   64
#define DTMF_TONE_MAX   4
#define VOL_LEVEL_MAX   5   /* AT+CLVL loudspeaker volume: 0..5 (4 = factory default) */
#define MIC_LEVEL_MAX   8   /* AT+CMICGAIN mic gain: 0..8 (3 = default) */
#define PB_NAME_MAX     32

#define READ_CHUNK      256
#define POLL_TIMEOUT_MS 20      /* max idle between timer ticks            */
#define IDLE_SLEEP_US   3000    /* fallback round-robin idle nap           */

#define CLIP_WAIT_MS      600   /* max wait for +CLIP after first RING      */
#define HB_INTERVAL_MS    10000 /* modem liveness heartbeat (AT)           */
#define HB_TIMEOUT_MS     3000
#define HB_MAX_FAIL       2     /* consecutive heartbeat misses → recover  */
#define READY_BEACON_MS   5000  /* periodic READY so a late FE re-syncs     */
#define REOPEN_BACKOFF_MS 200
#define REOPEN_BACKOFF_MAX 5000

#define GPS_POLL_MS     2000

/* Call state (single, shared — no fork)                               */
typedef struct {
    int  pending_clip;   /* RING seen, call not yet answered             */
    int  in_call;        /* call connected                               */
    int  outgoing;       /* call was initiated by CMD_DIAL               */
    int  local_hangup;   /* CMD_HANGUP issued                            */
    int  rejected;       /* CMD_REJECT issued                            */
    char caller_num[CALLER_NUM_MAX];
    char duration[16];
    int  last_cause;     /* most recent +CEER cause                      */
    int  ucs2_mode;      /* SMS charset is UCS2                          */
} call_state_t;

static call_state_t cs;

/* Last-known snapshot fields for READY/resync */
static char g_sim_state[16] = "UNKNOWN";
static int  g_net_stat = -1;
static int  g_rssi     = 99;
static int  g_bcl      = 100;

/* RING/CLIP debounce — send CALL_IN once, after CLIP or after CLIP_WAIT_MS */
static unsigned long g_clip_deadline = 0;
static int           g_call_in_sent  = 0;

/* Call-waiting deferred missed-call: when a +CCWA arrives during an active
 * call we save the caller number here; on call-end we log it as missed. */
static char g_pending_ccwa_num[CALLER_NUM_MAX];

/* GPS state */
static int           g_gps_on       = 0;
static unsigned long g_last_gps_poll = 0;

/* Polling throttle — cached last query time for CMD_BATTERY / CMD_SIGNAL */
static unsigned long g_last_cbc_ms  = 0;
static unsigned long g_last_csq_ms  = 0;

/* File descriptors + boot id                                          */
static int fd_sim = -1;
static int fd_fe = -1;

/* Modem lifecycle — declared early because dispatch_cmd() rejects commands
 * while the modem is not READY (see SECTION 10 for the recovery supervisor). */
enum { MODEM_INITIALIZING, MODEM_READY, MODEM_RECOVER };
static int g_modem_state = MODEM_INITIALIZING;

/* SECTION 0 — portable time + I/O primitives                         */

/* pd_now_ms() — monotonic millisecond clock.
 * On NothanOS (PD_NO_CLOCK): reads kernel jiffies via SYS_GET_TICK (22).
 * jiffies is incremented every 10 ms by the hardware DMTIMER ISR.
 * If the running kernel is too old to have SYS_GET_TICK (returns negative),
 * falls back to g_fallback_ms — a coarse counter advanced 3 ms per pd_wait(). */
static unsigned long pd_now_ms(void)
{
#ifdef PD_NO_CLOCK
    return getticks();   /* nothan_v2 syscall 19: milliseconds since boot */
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (unsigned long)ts.tv_sec * 1000UL + (unsigned long)(ts.tv_nsec / 1000000UL);
#endif
}

static void pd_sleep_us(unsigned us)
{
#ifdef PD_NO_USLEEP
    volatile unsigned i; for (i = 0; i < us * 10; i++) { }
#else
    usleep(us);
#endif
}

int open_uart(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    return fd;   /* baud/8N1 already set by the kernel UART driver */
}

/* Blocking-ish write with bounded retry (handles transient EAGAIN on a busy
 * tx FIFO). Returns 0 on full write, -1 on hard error. */
int write_all(int fd, const char *buf, size_t len)
{
    size_t done = 0;
    int    spins = 0;

    if (fd < 0) {
        return -1;
    }
    while (done < len) {
        ssize_t n = write(fd, buf + done, len - done);
        if (n > 0) {
            done += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            if (++spins > 200) {
                return -1;     /* ~200ms cap */
            }
            pd_sleep_us(1000);
            continue;
        }
        return -1;
    }
    return 0;
}

/* SECTION 1 — String helpers + UCS2/UTF-8 codec                      */

void extract_quoted(const char *p, char *out, int out_size)
{
    const char *q;
    int i;

    if (p == NULL || out == NULL || out_size <= 0) {
        return;
    }
    q = strchr(p, '"');
    if (!q) {
        out[0] = '\0';
        return;
    }
    q++;
    i = 0;
    while (i < out_size - 1 && *q && *q != '"') {
        out[i++] = *q++;
    }
    out[i] = '\0';
}

static const char *parse_int(const char *p, int *out)
{
    if (p == NULL || out == NULL) {
        return p;
    }
    while (*p == ' ') {
        p++;
    }
    *out = atoi(p);
    if (*p == '-') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        p++;
    }
    return p;
}

int is_imei_line(const char *s)
{
    int i;

    if (s == NULL) {
        return 0;
    }
    for (i = 0; i < 15; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
    }
    return (s[15] == '\0');
}

int parse_err_code(const char *p)
{
    if (p == NULL) {
        return -1;
    }
    while (*p == ' ') {
        p++;
    }
    if (*p < '0' || *p > '9') {
        return -1;
    }
    return atoi(p);
}

int hexv(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

int utf8_to_ucs2_hex(const char *in, char *out, int out_size)
{
    static const char H[] = "0123456789ABCDEF";
    const unsigned char *p = (const unsigned char *)in;
    int o = 0;

    while (*p) {
        unsigned int cp;
        if (*p < 0x80) {
            cp = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            if ((p[1] & 0xC0) != 0x80) {
                return -1;
            }
            cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) {
                return -1;
            }
            cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else {
            cp = '?';
            while ((*p & 0xC0) == 0x80) {
                p++;
            }
            if (*p) {
                p++;
            }
        }
        if (o + 4 >= out_size) {
            return -1;
        }
        out[o++] = H[(cp >> 12) & 0xF];
        out[o++] = H[(cp >>  8) & 0xF];
        out[o++] = H[(cp >>  4) & 0xF];
        out[o++] = H[cp & 0xF];
    }
    out[o] = '\0';
    return o;
}

int ucs2_hex_to_utf8(const char *in, char *out, int out_size)
{
    int o = 0;

    while (*in) {
        int h0, h1, h2, h3;
        while (*in && hexv(*in) < 0) {
            in++;
        }
        if (!*in) {
            break;
        }
        h0 = hexv(*in++);
        if (!*in) {
            break;
        }
        h1 = hexv(*in++);
        if (!*in) {
            break;
        }
        h2 = hexv(*in++);
        if (!*in) {
            break;
        }
        h3 = hexv(*in++);
        if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) {
            continue;
        }
        unsigned int cp = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
        if (cp < 0x80) {
            if (o + 1 >= out_size) {
                return -1;
            }
            out[o++] = (char)cp;
        } else if (cp < 0x800) {
            if (o + 2 >= out_size) {
                return -1;
            }
            out[o++] = (char)(0xC0 | (cp >> 6));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        } else {
            if (o + 3 >= out_size) {
                return -1;
            }
            out[o++] = (char)(0xE0 | (cp >> 12));
            out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    out[o] = '\0';
    return o;
}

int looks_like_ucs2_hex(const char *s)
{
    int len;

    if (s == NULL || *s == '\0') {
        return 0;
    }
    for (len = 0; s[len]; len++) {
        if (hexv(s[len]) < 0) {
            return 0;
        }
    }
    return len >= 8 && (len % 4) == 0;
}

/* Parse NMEA "DDMM.MMMMM" (or "DDDMM.MMMMM") + hemisphere → signed microdegrees.
 * hemi 'S' or 'W' negates. Returns 0 on success, -1 on bad input. */
static int parse_nmea_coord_1e6(const char *s, char hemi, int *out)
{
    int intpart = 0, frac = 0, frac_scale = 1;
    int deg, min_int, mult, min_1e6;

    if (!s || *s < '0' || *s > '9') {
        return -1;
    }
    while (*s >= '0' && *s <= '9') {
        intpart = intpart * 10 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9' && frac_scale < 100000) {
            frac = frac * 10 + (*s - '0');
            frac_scale *= 10;
            s++;
        }
    }
    deg     = intpart / 100;
    min_int = intpart % 100;
    mult    = 1000000 / frac_scale;          /* always integer: frac_scale is 10^n  */
    min_1e6 = (min_int * frac_scale + frac) * mult / 60;
    *out    = deg * 1000000 + min_1e6;
    if (hemi == 'S' || hemi == 'W') {
        *out = -(*out);
    }
    return 0;
}

/* Parse "DDD.d" string (one decimal place) → integer ×10. No libm needed. */
static int parse_fixed_1dec(const char *s)
{
    int v = 0, d = 0;

    if (!s) {
        return 0;
    }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        if (*s >= '0' && *s <= '9') {
            d = *s - '0';
        }
    }
    return v * 10 + d;
}











/* SECTION 2 — RPi4 frame raw writer */

void fe_raw_write(const char *json)
{
    uint8_t frame[PHONE_FRAME_MAX];
    int total;

    if (json == NULL) {
        return;
    }
    total = phone_frame_encode(frame, sizeof(frame), json, strlen(json));
    if (total < 0) {
        printf("[pd] fe> encode error\n");
        return;
    }
    if (write_all(fd_fe, (const char *)frame, (size_t)total) < 0) {
        printf("[pd] fe> frame write error\n");
    }
}

/* fire-and-forget (telemetry / non-critical) */
void fe_send(const char *json)
{
    if (json == NULL) {
        return;
    }
    printf("[pd] fe> %s\n", json);
    fe_raw_write(json);
}

/* SECTION 3 — reliability layer (BE→FE seq + retransmit) */

#define OUTBOX_CAP    8
#define REL_MAX_TRIES 6

typedef struct {
    char          json[PHONE_JSON_MAX];
    int           json_len;
    int           seq;
    int           tries;
    unsigned long next_at;
    int           active;
} rel_entry_t;

static rel_entry_t outbox[OUTBOX_CAP];
static int         be_seq = 0;

/* highest FE→BE reliable-command seq processed (dedup) */
static int         g_last_cmd_seq = 0;

/* FE peer state — tracked for logging */
static int         g_fe_boot = 0;

static unsigned long rel_backoff(int tries)
{
    unsigned long ms = 200;
    int i;
    for (i = 1; i < tries && i < 6; i++) ms *= 2;
    return ms > 4000 ? 4000 : ms;
}

static rel_entry_t *outbox_alloc(void)
{
    int i;
    for (i = 0; i < OUTBOX_CAP; i++)
        if (!outbox[i].active) return &outbox[i];
        
    return NULL;
}

/* Inject "seq":<n> into json (before closing '}') and retransmit until ACKed. */
void rel_send(const char *json, int json_len)
{
    unsigned long now = pd_now_ms();
    rel_entry_t  *e   = outbox_alloc();
    if (!e) {
        printf("[pd] outbox full — sending untracked\n");
        fe_raw_write(json);
        return;
    }
    be_seq++;
    {
        char aug[PHONE_JSON_MAX];
        int  base = json_len - 1;
        int  alen;
        while (base > 0 && json[base] != '}') base--;
        alen = snprintf(aug, sizeof(aug), "%.*s,\"seq\":%d}",
                        base, json, be_seq);
        if (alen < 0 || alen >= (int)sizeof(aug)) {
            fe_raw_write(json);
            return;
        }
        memcpy(e->json, aug, (size_t)(alen + 1));
        e->json_len = alen;
    }
    e->seq    = be_seq;
    e->tries  = 1;
    e->active = 1;
    e->next_at = now + rel_backoff(1);
    printf("[pd] fe> %s\n", e->json);
    fe_raw_write(e->json);
}

void rel_on_ack(int seq)
{
    int i;
    for (i = 0; i < OUTBOX_CAP; i++) {
        if (outbox[i].active && outbox[i].seq == seq) {
            printf("[pd] ACK seq=%d after %d tries\n", seq, outbox[i].tries);
            outbox[i].active = 0;
            return;
        }
    }
    printf("[pd] ACK seq=%d — no matching entry\n", seq);
}

void rel_retransmit_tick(unsigned long now)
{
    int i;
    for (i = 0; i < OUTBOX_CAP; i++) {
        rel_entry_t *e = &outbox[i];
        if (!e->active || now < e->next_at) {
            continue;
        }
        if (e->tries >= REL_MAX_TRIES) {
            printf("[pd] seq=%d unacked after %d tries — giving up\n",
                   e->seq, e->tries);
            e->active = 0;
            continue;
        }
        e->tries++;
        e->next_at = now + rel_backoff(e->tries);
        printf("[pd] retransmit seq=%d try=%d\n", e->seq, e->tries);
        fe_raw_write(e->json);
    }
}

/* Replay outbox entries with seq > last_seq (on FE reconnect / HELLO). */
void rel_replay_after(int last_seq)
{
    int i;
    for (i = 0; i < OUTBOX_CAP; i++) {
        if (!outbox[i].active) {
            continue;
        }
        if (outbox[i].seq <= last_seq) {
            continue;
        }
        printf("[pd] replay seq=%d on HELLO\n", outbox[i].seq);
        fe_raw_write(outbox[i].json);
    }
}

void send_ack(int seq)
{
    json_builder_t b;
    char json[64];
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_ACK);
    json_int(&b, "seq",  seq);
    json_end(&b);
    fe_send(json);
}

/* SECTION 4 — BBB → RPi4 frame senders */

static void fe_send_simple(const char *type)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    if (type == NULL) {
        return;
    }
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", type);
    json_end(&b);
    fe_send(json);
}

/* READY + state snapshot (also used as the periodic beacon). */
void send_ready_snapshot(void)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];
    const char *call = cs.in_call      ? "active"  :
                       cs.outgoing     ? "dialing" :
                       cs.pending_clip ? "ringing" : "idle";
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",    MSG_READY);
    json_int(&b, "seq",     be_seq);
    json_str(&b, "call",    call);
    if (cs.caller_num[0] && (cs.in_call || cs.pending_clip || cs.outgoing)) {
        json_str(&b, "callnum", cs.caller_num);
    }
    json_str(&b, "sim",     g_sim_state);
    json_int(&b, "net",     g_net_stat);
    json_int(&b, "rssi",    g_rssi);
    json_int(&b, "bcl",     g_bcl);
    json_int(&b, "ucs2",    cs.ucs2_mode);
    json_int(&b, "gps",     g_gps_on);
    json_end(&b);
    fe_raw_write(json);   /* routine beacon: send without logging (avoids console spam) */
}

static void fe_send_err(int code, const char *msg)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    if (msg == NULL) {
        msg = "error";
    }
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_ERR);
    json_int(&b, "code", code);
    json_str(&b, "msg",  msg);
    json_end(&b);
    fe_send(json);
}

/* ── critical events ── */
static void fe_send_call_in(const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_CALL_IN);
    json_str(&b, "num",  num ? num : "");
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

static void fe_send_call_miss(const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_CALL_MISS);
    json_str(&b, "num",  num ? num : "");
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

static void fe_send_call_end(const char *reason, const char *initiator,
                              const char *duration, int cause)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",      MSG_CALL_END);
    json_str(&b, "reason",    reason    ? reason    : "NO CARRIER");
    json_str(&b, "initiator", initiator ? initiator : "remote");
    json_str(&b, "duration",  duration  ? duration  : "");
    if (cause >= 0) {
        json_int(&b, "cause", cause);
    }
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

static void fe_send_sms_in(const char *from, const char *text, const char *ts)
{
    char json[PHONE_JSON_MAX];
    json_builder_t b;

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_SMS_IN);
    json_str(&b, "from", from ? from : "");
    json_str(&b, "text", text ? text : "");
    if (ts && *ts) {
        json_str(&b, "ts", ts);
    }
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

static void fe_send_modem_down(void)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_MODEM_DOWN);
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

static void fe_send_modem_up(void)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_MODEM_UP);
    json_end(&b);
    rel_send(json, (int)strlen(json));
}

/* ── non-critical events (fire-and-forget) ── */
static void fe_send_call_act(void)
{
    fe_send_simple(MSG_CALL_ACT);
}
static void fe_send_call_ring(void)
{
    fe_send_simple(MSG_CALL_RING);
}
static void fe_send_call_hold(void)
{
    fe_send_simple(MSG_CALL_HOLD);
}

static void fe_send_call_stat(const char *num, const char *raw)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_CALL_STAT);
    json_str(&b, "num",  num ? num : "");
    json_str(&b, "raw",  raw ? raw : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_call_wait(const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_CALL_WAIT);
    json_str(&b, "num",  num ? num : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_sms_stored(const char *mem, int index)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",  MSG_SMS_STORED);
    json_str(&b, "mem",   mem ? mem : "ME");
    json_int(&b, "index", index);
    json_end(&b);
    fe_send(json);
}

static void fe_send_sms_deliver(int ref, int status)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",   MSG_SMS_DELIVER);
    json_int(&b, "ref",    ref);
    json_int(&b, "status", status);
    json_end(&b);
    fe_send(json);
}

/* SMS send result — carries the FE's correlation id (cid) so the FE can flip
 * the right outgoing message sending→sent/failed. cid<0 → omit. */
static void fe_send_sms_ack(int ref, int cid)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_SMS_ACK);
    json_int(&b, "ref",  ref);
    if (cid >= 0) {
        json_int(&b, "cid", cid);
    }
    json_end(&b);
    fe_send(json);
}

static void fe_send_sms_err(int code, const char *msg, int cid)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_SMS_ERR);
    json_int(&b, "code", code);
    json_str(&b, "msg",  msg ? msg : "send failed");
    if (cid >= 0) {
        json_int(&b, "cid", cid);
    }
    json_end(&b);
    fe_send(json);
}

static void fe_send_sms_list(int index, const char *stat, const char *from,
                              const char *ts, const char *text)
{
    char json[PHONE_JSON_MAX];
    json_builder_t b;
    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",  MSG_SMS_LIST);
    json_int(&b, "index", index);
    json_str(&b, "stat",  stat ? stat : "");
    json_str(&b, "from",  from ? from : "");
    json_str(&b, "ts",    ts   ? ts   : "");
    json_str(&b, "text",  text ? text : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_signal(int rssi, int ber)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_SIGNAL);
    json_int(&b, "rssi", rssi);
    json_int(&b, "ber",  ber);
    json_end(&b);
    fe_send(json);
}

static void fe_send_net_reg(int stat, const char *lac, const char *ci)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_NET_REG);
    json_int(&b, "stat", stat);
    if (lac && *lac) {
        json_str(&b, "lac", lac);
    }
    if (ci  && *ci ) {
        json_str(&b, "ci",  ci);
    }
    json_end(&b);
    fe_send(json);
}

static void fe_send_gprs_reg(int stat)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_GPRS_REG);
    json_int(&b, "stat", stat);
    json_end(&b);
    fe_send(json);
}

static void fe_send_net_opr(const char *name)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_NET_OPR);
    json_str(&b, "name", name ? name : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_ussd_resp(int code, const char *text)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_USSD_RESP);
    json_int(&b, "code", code);
    json_str(&b, "text", text ? text : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_timezone(const char *tz, int dst)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_TIMEZONE);
    json_str(&b, "tz",   tz ? tz : "");
    if (dst >= 0) {
        json_int(&b, "dst", dst);
    }
    json_end(&b);
    fe_send(json);
}

static void fe_send_clock(const char *ts)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_CLOCK);
    json_str(&b, "ts",   ts ? ts : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_sim_stat(const char *state)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",  MSG_SIM_STAT);
    json_str(&b, "state", state ? state : "UNKNOWN");
    json_end(&b);
    fe_send(json);
}

static void fe_send_battery(int bcs, int bcl)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_BATTERY);
    json_int(&b, "bcs",  bcs);
    json_int(&b, "bcl",  bcl);
    json_end(&b);
    fe_send(json);
}

static void fe_send_imei(const char *id)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_IMEI);
    json_str(&b, "id",   id ? id : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_iccid(const char *id)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_ICCID);
    json_str(&b, "id",   id ? id : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_phone_num(const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_PHONE_NUM);
    json_str(&b, "num",  num ? num : "");
    json_end(&b);
    fe_send(json);
}

static void fe_send_radio_state(int fun)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_RADIO_STATE);
    json_int(&b, "fun",  fun);
    json_end(&b);
    fe_send(json);
}

static void fe_send_pb_entry(int index, const char *num, const char *name, int type)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",  MSG_PB_ENTRY);
    json_int(&b, "index", index);
    json_str(&b, "num",   num  ? num  : "");
    json_str(&b, "name",  name ? name : "");
    json_int(&b, "atype", type);
    json_end(&b);
    fe_send(json);
}

static void fe_send_ss_notify(const char *dir, int code, const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_SS_NOTIFY);
    json_str(&b, "dir",  dir ? dir : "");
    json_int(&b, "code", code);
    if (num && *num) {
        json_str(&b, "num", num);
    }
    json_end(&b);
    fe_send(json);
}

static void fe_send_cf_state(int reason, int status, const char *num)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",   MSG_CF_STATE);
    json_int(&b, "reason", reason);
    json_int(&b, "status", status);
    if (num && *num) {
        json_str(&b, "num", num);
    }
    json_end(&b);
    fe_send(json);
}

static void rpi_send_gps_state(int on)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type", MSG_GPS_STATE);
    json_int(&b, "on", on);
    json_end(&b);
    fe_send(json);
}

/* lat/lon: microdegrees (×1e6).  alt: decimeters (×10).
 * speed10: km/h ×10.  course10: degrees ×10. */
static void rpi_send_gps_loc(int lat_1e6, int lon_1e6, int alt_dm,
                              int speed_10, int course_10, int fix)
{
    json_builder_t b;
    char json[PHONE_JSON_MAX];

    json_begin(&b, json, sizeof(json));
    json_str(&b, "type",     MSG_GPS_LOC);
    json_int(&b, "fix",      fix);
    json_int(&b, "lat",      lat_1e6);
    json_int(&b, "lon",      lon_1e6);
    if (fix) {
        json_int(&b, "alt",      alt_dm);
        json_int(&b, "speed10",  speed_10);
        json_int(&b, "course10", course_10);
    }
    json_end(&b);
    fe_send(json);
}

/* SECTION 5 — AT command/response state machine                      */

typedef enum { WAIT_FINAL, WAIT_PROMPT, WAIT_SMS_RESULT, WAIT_NONE } at_wait_t;

/* step kinds — route completion side effects */
enum { STEP_PLAIN, STEP_HEARTBEAT, STEP_SMS_HDR, STEP_SMS_RESULT,
       STEP_SMS_LIST, STEP_PB_READ, STEP_NET_SCAN, STEP_HANGUP, STEP_DIAL,
       STEP_SMS_MODE_RESTORE };   /* AT+CMGF=1 sentinel after multipart PDU send */

#define SMS_UCS2_SINGLE_MAX  70   /* chars: single UCS2 SMS, no UDH        */
#define SMS_UCS2_PART_CHARS  67   /* chars per part in concatenated UCS2   */
#define SMS_CONCAT_PARTS_MAX  8   /* max parts supported                   */

static unsigned char g_sms_concat_ref = 1;

/* ── Incoming multipart SMS reassembly ───────────────────────────────
 * Text mode (CMGF=1) delivers each part as a separate +CMT URC with no
 * UDH exposed. We buffer parts from the same sender in a short time
 * window (SMS_CONCAT_COLLECT_MS after the last received part) and emit
 * one combined SMS_IN when the window expires. */
#define SMS_CONCAT_IN_MAX       4       /* concurrent senders in flight  */
#define SMS_CONCAT_COLLECT_MS   3000    /* window after last part arrival; Viettel SMSC delivers parts ~2s apart */
#define SMS_CONCAT_TEXT_CAP     2048    /* max total reassembled UTF-8   */

typedef struct {
    int   active;
    char  sender[CALLER_NUM_MAX];
    char  ts[24];
    char  text[SMS_CONCAT_TEXT_CAP];
    int   text_len;
    unsigned long expire_ms;
} SmsConcatIn;

static SmsConcatIn g_sms_concat_in[SMS_CONCAT_IN_MAX];

/* PDU hex body ≤ 310 B; text-mode SMS ≤ 162 B — PHONE_JSON_MAX bloats at_q to 65 KB BSS */
#define AT_TEXT_MAX 1100   

typedef struct {
    char          text[AT_TEXT_MAX];
    int           textlen;
    at_wait_t     wait;
    unsigned long timeout_ms;
    int           ctx;                   /* correlation id (sms cid) or -1     */
    int           kind;
} at_step_t;

#define AT_QUEUE_MAX 32
static at_step_t at_q[AT_QUEUE_MAX];
static int  at_head = 0, at_tail = 0, at_count = 0;

static int           at_busy = 0;
static at_wait_t     at_wait = WAIT_NONE;
static unsigned long at_deadline = 0;
static int           at_cur_ctx  = -1;
static int           at_cur_kind = STEP_PLAIN;

static int g_hb_fail = 0;

void sim_raw_write(const char *s, int len)
{
    if (s == NULL || len <= 0) {
        return;
    }
    if (len == 1 && s[0] == '\r') {
        printf("[pd] sim> <CR>\n");
    } else if (len == 1 && s[0] == 0x1A) {
        printf("[pd] sim> <CTRL-Z>\n");
    } else if (len == 1 && s[0] == 0x1B) {
        printf("[pd] sim> <ESC>\n");
    } else if (!(len == 3 && s[0] == 'A' && s[1] == 'T' && s[2] == '\r')) {
        printf("[pd] sim> %.*s\n", len, s);
    }
    write_all(fd_sim, s, (size_t)len);
}

/* Enqueue a step. text may be a C-string OR contain embedded NUL/Ctrl-Z, so a
 * length is given explicitly (use -1 for strlen). */
int at_enqueue(const char *text, int len, at_wait_t wait,
                      unsigned long timeout, int kind, int ctx)
{
    at_step_t *s;
    if (at_count >= AT_QUEUE_MAX) {
        printf("[pd] queue full — dropping command\n");
        return -1;
    }
    s = &at_q[at_tail];
    if (len < 0) {
        len = text ? (int)strlen(text) : 0;
    }
    if (len > (int)sizeof(s->text)) {
        len = (int)sizeof(s->text);
    }
    if (text && len > 0) {
        memcpy(s->text, text, (size_t)len);
    }
    s->textlen   = len;
    s->wait      = wait;
    s->timeout_ms= timeout;
    s->kind      = kind;
    s->ctx       = ctx;
    at_tail = (at_tail + 1) % AT_QUEUE_MAX;
    at_count++;
    return 0;
}

int at_free_slots(void)
{
    return AT_QUEUE_MAX - at_count;
}

/* Forward declaration (defined in SECTION 10 — init / recovery). */
void trigger_recover(const char *why);

int at_submit(const char *cmd, unsigned long timeout)
{
    int rc = at_enqueue(cmd, -1, WAIT_FINAL, timeout, STEP_PLAIN, -1);
    if (rc < 0) {
        printf("[pd] AT queue full — dropping: %s\n", cmd);
        /* Recovery will flush the queue and re-init the modem. Trigger it
         * immediately so no future commands are lost. */
        trigger_recover("at queue overflow");
    }
    return rc;
}

void at_pump(void)
{
    while (!at_busy && at_count > 0) {
        at_step_t *s = &at_q[at_head];
        at_head = (at_head + 1) % AT_QUEUE_MAX;
        at_count--;

        if (s->textlen > 0) sim_raw_write(s->text, s->textlen);

        if (s->wait == WAIT_NONE) {
            continue;   /* fire-and-forget, next step */
        }

        at_busy     = 1;
        at_wait     = s->wait;
        at_deadline = pd_now_ms() + s->timeout_ms;
        at_cur_ctx  = s->ctx;
        at_cur_kind = s->kind;
    }
}

void handle_call_release(const char *reason);    /* fwd */
void at_cancel_queued_sms(void);                 /* fwd */
static int  at_has_more_sms_result(void);               /* fwd */
void handle_sms_body(const char *raw);           /* fwd (needed by sms_body_hex_flush) */

/* Drop the next queued step if it matches `kind`. Used to cancel the SMS
 * body after its header failed — otherwise raw hex+Ctrl-Z would be written
 * to the modem as if it were an AT command, causing full pipeline desync. */
void at_drop_next_if(int kind)
{
    if (at_count > 0 && at_q[at_head].kind == kind) {
        printf("[pd] dropping orphaned step kind=%d (parent failed)\n", kind);
        at_head = (at_head + 1) % AT_QUEUE_MAX;
        at_count--;
    }
}

void at_finish(int ok)
{
    int kind = at_cur_kind;
    at_busy = 0; at_wait = WAIT_NONE; at_cur_kind = STEP_PLAIN; at_cur_ctx = -1;

    if (kind == STEP_HEARTBEAT) {
        if (ok) {
            g_hb_fail = 0;
        } else if (++g_hb_fail >= HB_MAX_FAIL) {
            trigger_recover("heartbeat timeout");
            return;
        }
    } else if (kind == STEP_HANGUP && ok) {
        /* AT+CHUP returned OK. If the modem didn't emit VOICE CALL: END / NO CARRIER
         * (common when hanging up an outgoing call in alerting state), force-clear
         * the call state so the app gets CALL_END and stops retrying CMD_HANGUP. */
        if (cs.in_call || cs.pending_clip || cs.outgoing) {
            handle_call_release("AT+CHUP");
        }
    } else if (kind == STEP_SMS_MODE_RESTORE) {
        printf("[pd] PDU mode restored to text mode%s\n", ok ? "" : " (failed)");
    } else if (kind == STEP_SMS_LIST) {
        fe_send_simple(MSG_SMS_LIST_END);
    } else if (kind == STEP_PB_READ) {
        fe_send_simple(MSG_PB_ENTRY_END);
    } else if (kind == STEP_NET_SCAN) {
        fe_send_simple(MSG_NET_SCAN_END);
    } else if (kind == STEP_DIAL && !ok) {
        /* ATD ERROR or timeout fallback. BUSY/NO CARRIER URC normally arrives
         * first and handle_call_release() clears cs.outgoing — only send
         * CALL_END here if the URC never came (e.g. no network, modem glitch). */
        if (cs.outgoing && !cs.in_call) {
            printf("[pd] dial failed (no URC) — clearing outgoing state\n");
            cs.outgoing = 0;
            fe_send_call_end("DIAL FAILED", "local", "", -1);
            cs.caller_num[0] = '\0';
        }
    }
    at_pump();
}

int is_final_error(const char *line)
{
    return strcmp(line, "ERROR") == 0 ||
           strncmp(line, "+CME ERROR:", 11) == 0 ||
           strncmp(line, "+CMS ERROR:", 11) == 0;
}

/* Consume an AT result line (OK / ERROR / +CMGS:). Returns nothing. */
void at_consume(const char *line)
{
    if (!at_busy) {
        return;
    }

    if (at_wait == WAIT_FINAL) {
        if (strcmp(line, "OK") == 0) {
            at_finish(1);
        } else if (is_final_error(line)) {
            at_finish(0);
        }
        /* else: intermediate response line — keep waiting */
    } else if (at_wait == WAIT_PROMPT) {
        /* Modem rejected AT+CMGS before showing the '>' prompt. */
        if (is_final_error(line)) {
            int code = (strncmp(line, "+CMS ERROR:", 11) == 0 ||
                        strncmp(line, "+CME ERROR:", 11) == 0)
                       ? parse_err_code(line + 11) : -1;
            printf("[pd] CMGS header rejected: %s\n", line);
            fe_send_sms_err(code, line, at_cur_ctx);
            at_drop_next_if(STEP_SMS_RESULT);
            at_cancel_queued_sms();   /* drop remaining parts (no-op for single SMS) */
            at_finish(0);
        }
    } else if (at_wait == WAIT_SMS_RESULT) {
        if (strncmp(line, "+CMGS:", 6) == 0) {
            int ref = 0; parse_int(line + 6, &ref);
            if (!at_has_more_sms_result())   /* only ACK on last part */
                fe_send_sms_ack(ref, at_cur_ctx);
            at_finish(1);
        } else if (is_final_error(line)) {
            int code = parse_err_code(line + 11);
            fe_send_sms_err(code, line, at_cur_ctx);
            at_cancel_queued_sms();   /* drop remaining parts */
            at_finish(0);
        }
    }
    /* WAIT_PROMPT resolves in the byte-level prompt detector. */
}

void at_timers_tick(unsigned long now)
{
    if (!at_busy || now < at_deadline) {
        return;
    }

    if (at_wait == WAIT_PROMPT && at_cur_kind == STEP_SMS_HDR) {
        sim_raw_write("\x1B", 1);
        fe_send_sms_err(-1, "prompt timeout", at_cur_ctx);
        at_drop_next_if(STEP_SMS_RESULT);
        at_cancel_queued_sms();
    } else if (at_wait == WAIT_SMS_RESULT) {
        sim_raw_write("\x1B", 1);
        fe_send_sms_err(-1, "send timeout", at_cur_ctx);
        at_cancel_queued_sms();
    }
    at_finish(0);
}

/* SECTION 6 — SIM URC + response line handlers */

/* Pending SMS body read (event-driven; replaces the old blocking read) */
enum { BODY_NONE, BODY_CMT, BODY_CMGL };
static int  g_body_pending = BODY_NONE;
static char g_body_sender[CALLER_NUM_MAX];
static char g_body_ts[24];
static char g_body_stat[16];
static int  g_body_index;
static int  g_body_ucs2;

/* Multi-line hex body accumulation.
 * SIM7600CE text mode wraps long UCS2 hex bodies at its internal line-output
 * limit (~160 chars), so a 280-char UCS2 body arrives as two consecutive hex
 * lines (both without a +CMT: prefix).  We accumulate all-hex continuation
 * lines and flush the complete body after BODY_HEX_FLUSH_MS of idle silence
 * or when the next non-hex line terminates the sequence. */
#define BODY_HEX_FLUSH_MS  250
static char  g_body_hex_acc[SMS_HEX_MAX + 4];
static int   g_body_hex_acc_len;
static unsigned long g_body_hex_flush_ms;

int is_all_hex_line(const char *s)
{
    int len = 0;
    if (!s || !*s) {
        return 0;
    }
    while (*s) {
        char c = *s++;
        if (!((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'))) {
            return 0;
        }
        len++;
    }
    return len > 0;
}

/* Flush accumulated hex body (if any) through handle_sms_body(). */
void sms_body_hex_flush(void)
{
    if (g_body_hex_acc_len > 0 && g_body_pending == BODY_CMT) {
        g_body_hex_acc[g_body_hex_acc_len] = '\0';
        handle_sms_body(g_body_hex_acc);   /* clears g_body_pending */
        g_body_hex_acc_len = 0;
    } else {
        g_body_hex_acc_len = 0;
        g_body_pending     = BODY_NONE;
    }
    g_body_hex_flush_ms = 0;
}

void handle_ring(void)
{
    /* Ignore RING if already in a call or dialing (call waiting is handled
     * separately via +CCWA — we log a missed call when the active call ends). */
    if (cs.in_call || cs.outgoing || cs.pending_clip) {
        printf("[pd] RING ignored (in_call=%d outgoing=%d pending_clip=%d)\n",
               cs.in_call, cs.outgoing, cs.pending_clip);
        return;
    }
    cs.pending_clip   = 1;
    g_call_in_sent    = 0;
    g_clip_deadline   = pd_now_ms() + CLIP_WAIT_MS;
    fe_send_call_ring();
}

void handle_clip(const char *line)
{
    char num[CALLER_NUM_MAX];
    if (!cs.pending_clip) {
        return;
    }
    extract_quoted(line + 6, num, sizeof(num));
    strncpy(cs.caller_num, num, sizeof(cs.caller_num) - 1);
    cs.caller_num[sizeof(cs.caller_num) - 1] = '\0';
    if (!g_call_in_sent) {
        g_call_in_sent  = 1;
        g_clip_deadline = 0;   /* cancel fallback */
        fe_send_call_in(num);
    }
}

/* Fallback: if CLIP never arrived before the deadline, send CALL_IN with
 * whatever number we have (possibly empty — caller withheld or no CLIP). */
void clip_timer_tick(unsigned long now)
{
    if (!cs.pending_clip || g_call_in_sent) {
        return;
    }
    if (g_clip_deadline == 0 || now < g_clip_deadline) {
        return;
    }
    g_call_in_sent  = 1;
    g_clip_deadline = 0;
    printf("[pd] CLIP timeout — sending CALL_IN with num='%s'\n", cs.caller_num);
    fe_send_call_in(cs.caller_num);
}

void handle_voice_call_begin(void)
{
    cs.in_call = 1; cs.pending_clip = 0; cs.duration[0] = '\0';
    if (cs.outgoing) {
        /* SIM7600CE: VOICE CALL: BEGIN = bên kia đã answer thật sự.
         * +COLP chỉ confirm số, không confirm answer → ignore COLP cho CALL_ACT */
        cs.outgoing = 0;
        printf("[pd] voice call active\n");
        fe_send_call_act();
    } else {
        printf("[pd] voice call active (incoming)\n");
        fe_send_call_act();
    }
}

void handle_colp(void)
{
    (void)0;
}


void handle_voice_call_end(const char *line)
{
    const char *p = line; int i = 0;
    while (*p == ' ') {
        p++;
    }
    while (*p && i < (int)(sizeof(cs.duration) - 1)) {
        cs.duration[i++] = *p++;
    }
    cs.duration[i] = '\0';
    printf("[pd] voice call ended, duration=%s\n", cs.duration);
}

void handle_call_release(const char *reason)
{
    if (cs.in_call || cs.local_hangup || cs.rejected || cs.outgoing) {
        const char *initiator; const char *r = reason;
        if (cs.rejected)           { initiator = "local"; r = "REJECTED"; }
        else if (cs.local_hangup)    initiator = "local";
        else                         initiator = "remote";
        fe_send_call_end(r, initiator, cs.duration, cs.last_cause);
        at_submit("AT+CEER\r", 2000);

        /* If someone called while we were busy (+CCWA), log them as missed now. */
        if (g_pending_ccwa_num[0]) {
            printf("[pd] CCWA caller %s logged as missed (was busy)\n", g_pending_ccwa_num);
            fe_send_call_miss(g_pending_ccwa_num);
            g_pending_ccwa_num[0] = '\0';
        }
    } else if (cs.pending_clip) {
        fe_send_call_miss(cs.caller_num);
    }
    cs.in_call = 0; cs.pending_clip = 0; cs.outgoing = 0;
    cs.local_hangup = 0; cs.rejected = 0;
    cs.last_cause = -1; cs.caller_num[0] = '\0'; cs.duration[0] = '\0';
    g_call_in_sent = 0; g_clip_deadline = 0;
}

void handle_clcc(const char *line)
{
    char num[CALLER_NUM_MAX];
    extract_quoted(line + 6, num, sizeof(num));
    fe_send_call_stat(num, line + 6);
}

void handle_ccwa(const char *line)
{
    char num[CALLER_NUM_MAX];
    extract_quoted(line + 6, num, sizeof(num));
    /* Save the caller number so we can log it as a missed call when the
     * active call ends (no hold/swap in this demo — the 3rd party gets busy). */
    strncpy(g_pending_ccwa_num, num, sizeof(g_pending_ccwa_num) - 1);
    g_pending_ccwa_num[sizeof(g_pending_ccwa_num) - 1] = '\0';
    printf("[pd] CCWA from %s (will log missed on call-end)\n", num);
}

void handle_ceer(const char *line)
{
    int code = -1;
    parse_int(line + 6, &code);
    if (code != 0) {
        cs.last_cause = code;
    }
    printf("[pd] +CEER cause=%d\n", code);
}

static void handle_cssi(const char *line)
{
    int code = 0; parse_int(line + 6, &code);
    fe_send_ss_notify("out", code, NULL);
}

static void handle_cssu(const char *line)
{
    int code = 0; char num[CALLER_NUM_MAX]; const char *p = line + 6;
    parse_int(p, &code);
    num[0] = '\0';
    p = strchr(p, '"');
    if (p) {
        extract_quoted(p, num, sizeof(num));
    }
    fe_send_ss_notify("in", code, num);
}

static void handle_ccfc(const char *line)
{
    int status = 0; char num[CALLER_NUM_MAX]; const char *p = line + 6;
    parse_int(p, &status);
    num[0] = '\0';
    p = strchr(p, '"');
    if (p) {
        extract_quoted(p, num, sizeof(num));
    }
    fe_send_cf_state(0, status, num);
}

/* ── Incoming concat reassembly helpers ──────────────────────────── */

static SmsConcatIn *sms_ci_find(const char *sender)
{
    int i;
    for (i = 0; i < SMS_CONCAT_IN_MAX; i++) {
        if (g_sms_concat_in[i].active &&
            strncmp(g_sms_concat_in[i].sender, sender, CALLER_NUM_MAX - 1) == 0) {
            return &g_sms_concat_in[i];
        }
    }
    return NULL;
}

static SmsConcatIn *sms_ci_alloc(const char *sender, const char *ts)
{
    int i;
    SmsConcatIn *oldest = NULL;
    for (i = 0; i < SMS_CONCAT_IN_MAX; i++) {
        if (!g_sms_concat_in[i].active) {
            SmsConcatIn *sc = &g_sms_concat_in[i];
            sc->active   = 1;
            sc->text_len = 0;
            sc->text[0]  = '\0';
            strncpy(sc->sender, sender, sizeof(sc->sender) - 1);
            sc->sender[sizeof(sc->sender) - 1] = '\0';
            strncpy(sc->ts, ts ? ts : "", sizeof(sc->ts) - 1);
            sc->ts[sizeof(sc->ts) - 1] = '\0';
            return sc;
        }
        if (!oldest || g_sms_concat_in[i].expire_ms < oldest->expire_ms) {
            oldest = &g_sms_concat_in[i];
        }
    }
    /* evict oldest — flush it first */
    if (oldest) {
        printf("[pd] evict sender=%s to make room\n", oldest->sender);
        fe_send_sms_in(oldest->sender, oldest->text, oldest->ts);
        oldest->active   = 1;
        oldest->text_len = 0;
        oldest->text[0]  = '\0';
        strncpy(oldest->sender, sender, sizeof(oldest->sender) - 1);
        oldest->sender[sizeof(oldest->sender) - 1] = '\0';
        strncpy(oldest->ts, ts ? ts : "", sizeof(oldest->ts) - 1);
        oldest->ts[sizeof(oldest->ts) - 1] = '\0';
        return oldest;
    }
    return NULL;
}

static void sms_ci_append(SmsConcatIn *sc, const char *text)
{
    int tlen = (int)strlen(text);
    if (sc->text_len + tlen >= SMS_CONCAT_TEXT_CAP - 1) {
        printf("[pd] concat buffer full, truncating\n");
        tlen = SMS_CONCAT_TEXT_CAP - 1 - sc->text_len;
        if (tlen <= 0) {
            return;
        }
    }
    memcpy(sc->text + sc->text_len, text, (size_t)tlen);
    sc->text_len += tlen;
    sc->text[sc->text_len] = '\0';
}

static void sms_ci_flush(SmsConcatIn *sc)
{
    if (sc->active && sc->text_len > 0) {
        fe_send_sms_in(sc->sender, sc->text, sc->ts);
    }
    sc->active   = 0;
    sc->text_len = 0;
    sc->text[0]  = '\0';
}

void sms_ci_tick(unsigned long now)
{
    int i;
    for (i = 0; i < SMS_CONCAT_IN_MAX; i++) {
        if (g_sms_concat_in[i].active && now >= g_sms_concat_in[i].expire_ms) {
            sms_ci_flush(&g_sms_concat_in[i]);
        }
    }
}

/* +CMT header → next non-empty line is the body (set pending). */
void handle_cmt_header(const char *line)
{
    /* UCS2 phone number: up to ~21 digits × 4 hex chars = 84 chars */
    char sender_raw[96]; const char *p;
    extract_quoted(line + 5, sender_raw, sizeof(sender_raw));
    if (cs.ucs2_mode && looks_like_ucs2_hex(sender_raw)) {
        int dlen = ucs2_hex_to_utf8(sender_raw, g_body_sender, sizeof(g_body_sender));
        if (dlen < 0) {
            strncpy(g_body_sender, sender_raw, sizeof(g_body_sender) - 1);
            g_body_sender[sizeof(g_body_sender) - 1] = '\0';
        }
    } else {
        strncpy(g_body_sender, sender_raw, sizeof(g_body_sender) - 1);
        g_body_sender[sizeof(g_body_sender) - 1] = '\0';
    }
    g_body_ucs2 = cs.ucs2_mode;   /* body UCS2 check; handle_sms_body also auto-detects */
    g_body_ts[0] = '\0';
    p = strchr(line + 5, ',');
    if (p) {
        const char *q = p; int comma_n = 0;
        while (*q && comma_n < 2) { if (*q == ',') comma_n++; q++; }
        if (comma_n == 2) extract_quoted(q, g_body_ts, sizeof(g_body_ts));
    }
    g_body_pending = BODY_CMT;
}

/* +CMGL / +CMGR header → next non-empty line is the body. */
void handle_cmgl_header(const char *line)
{
    char from_raw[96]; const char *p = line + 6;
    g_body_index = 0; g_body_stat[0] = '\0'; g_body_ts[0] = '\0';
    parse_int(p, &g_body_index);
    p = strchr(p, ',');
    if (!p) {
        g_body_pending = BODY_CMGL;
        g_body_sender[0] = '\0';
        return;
    }
    extract_quoted(p + 1, g_body_stat, sizeof(g_body_stat));
    p = strchr(p + 1, ',');
    if (!p) {
        g_body_pending = BODY_CMGL;
        g_body_sender[0] = '\0';
        return;
    }
    extract_quoted(p + 1, from_raw, sizeof(from_raw));
    {
        int commas = 0;
        while (*p && commas < 3) {
            if (*p == ',') {
                commas++;
            }
            p++;
        }
        extract_quoted(p, g_body_ts, sizeof(g_body_ts));
    }
    if (cs.ucs2_mode && looks_like_ucs2_hex(from_raw)) {
        int dlen = ucs2_hex_to_utf8(from_raw, g_body_sender, sizeof(g_body_sender));
        if (dlen < 0) {
            strncpy(g_body_sender, from_raw, sizeof(g_body_sender) - 1);
            g_body_sender[sizeof(g_body_sender) - 1] = '\0';
        }
    } else {
        strncpy(g_body_sender, from_raw, sizeof(g_body_sender) - 1);
        g_body_sender[sizeof(g_body_sender) - 1] = '\0';
    }
    g_body_ucs2 = cs.ucs2_mode;
    g_body_pending = BODY_CMGL;
}

void handle_sms_body(const char *raw)
{
    /* raw is the +CMT body or the accumulated UCS2 hex — both bounded by the
     * hex accumulator (SMS_HEX_MAX). Static buffer instead of malloc (no heap). */
    static char text[SMS_HEX_MAX + 8];
    int   raw_len = raw ? (int)strlen(raw) : 0;
    int   buf_sz  = raw_len + 4;
    int   kind    = g_body_pending;
    g_body_pending = BODY_NONE;

    if (buf_sz > (int)sizeof(text)) buf_sz = (int)sizeof(text);

    int is_ucs2 = g_body_ucs2 || looks_like_ucs2_hex(raw);
    if (is_ucs2) {
        int dlen = ucs2_hex_to_utf8(raw, text, buf_sz);
        if (dlen < 0) {
            strncpy(text, raw, (size_t)(buf_sz - 1)); text[buf_sz - 1] = '\0';
        } else {
            /* Replace embedded U+0000 nulls (decoded as '\0') so downstream
             * strlen() doesn't silently truncate the message. */
            int j;
            for (j = 0; j < dlen; j++) {
                if (text[j] == '\0') {
                    text[j] = ' ';
                }
            }
        }
    } else {
        strncpy(text, raw, (size_t)(buf_sz - 1)); text[buf_sz - 1] = '\0';
    }

    if (kind == BODY_CMT) {
        /* Buffer in concat window so multipart SMS arrive as one message.
         * Single-part SMS is emitted after SMS_CONCAT_COLLECT_MS of silence
         * from this sender — acceptable latency in exchange for correct
         * reassembly of multipart messages delivered in text mode (no UDH). */
        SmsConcatIn *sc = sms_ci_find(g_body_sender);
        if (!sc) {
            sc = sms_ci_alloc(g_body_sender, g_body_ts);
        }
        if (sc) {
            sms_ci_append(sc, text);
            sc->expire_ms = pd_now_ms() + SMS_CONCAT_COLLECT_MS;
        } else {
            fe_send_sms_in(g_body_sender, text, g_body_ts);
        }
    } else {
        fe_send_sms_list(g_body_index, g_body_stat, g_body_sender, g_body_ts, text);
    }
}

void handle_cmti(const char *line)
{
    char mem[8]; const char *p; int index = 0;
    extract_quoted(line + 6, mem, sizeof(mem));
    p = strchr(line + 6, ',');
    if (p) {
        parse_int(p + 1, &index);
    }
    fe_send_sms_stored(mem, index);
}

void handle_cds(const char *line)
{
    const char *p = line + 4;
    int ref = 0;
    int status = 0;
    int val;

    p = parse_int(p, &val);
    if (*p == ',') {
        p++;
    }
    p = parse_int(p, &ref);
    {
        int commas = 0;
        while (*p && commas < 5) {
            if (*p == ',') {
                commas++;
            }
            p++;
        }
    }
    parse_int(p, &status);
    fe_send_sms_deliver(ref, status);
}

void handle_csq(const char *line)
{
    const char *p, *comma; int rssi, ber = 0;
    p = parse_int(line + 5, &rssi);
    comma = strchr(p, ',');
    if (comma) {
        parse_int(comma + 1, &ber);
    }
    g_rssi = rssi;
    fe_send_signal(rssi, ber);
}

/* Count commas in s to distinguish URC vs query-response format:
 *   0 commas: +CREG: <stat>
 *   1 comma:  +CREG: <n>,<stat>            (query, no location)
 *   2 commas: +CREG: <stat>,<lac>,<ci>     (URC with location)
 *   3+ commas:+CREG: <n>,<stat>,<lac>,<ci> (query with location) */
static int count_commas(const char *s)
{
    int n = 0;
    while (*s) {
        if (*s == ',') {
            n++;
        }
        s++;
    }
    return n;
}

void handle_creg(const char *line)
{
    const char *p = line + 6, *c1, *c2;
    int nc = count_commas(p), v1 = 0, stat = 0;
    char lac[8] = {0}, ci[12] = {0};

    parse_int(p, &v1);
    c1 = strchr(p, ',');

    if (nc == 0) {
        stat = v1;
    } else if (nc == 1) {
        /* <n>,<stat> — query, no location */
        if (c1) {
            parse_int(c1 + 1, &stat);
        } else {
            stat = v1;
        }
    } else if (nc == 2) {
        /* <stat>,<lac>,<ci> — URC with location */
        stat = v1;
        c2 = c1 ? strchr(c1 + 1, ',') : NULL;
        if (c1) {
            extract_quoted(c1, lac, sizeof(lac));
        }
        if (c2) {
            extract_quoted(c2, ci,  sizeof(ci));
        }
    } else {
        /* <n>,<stat>,<lac>,<ci>[,<AcT>] — query with location */
        if (c1) {
            parse_int(c1 + 1, &stat);
            c2 = strchr(c1 + 1, ',');
        } else {
            stat = v1;
            c2 = NULL;
        }
        if (c2) {
            const char *c3 = strchr(c2 + 1, ',');
            extract_quoted(c2, lac, sizeof(lac));
            if (c3) {
                extract_quoted(c3, ci, sizeof(ci));
            }
        }
    }
    g_net_stat = stat;
    fe_send_net_reg(stat, lac, ci);
}

void handle_cgreg(const char *line)
{
    const char *p = line + 7, *c1;
    int nc = count_commas(p), v1 = 0, stat = 0;

    parse_int(p, &v1);
    c1 = strchr(p, ',');

    if (nc == 0) {
        stat = v1;
    } else if (nc == 1) {
        if (c1) {
            parse_int(c1 + 1, &stat);
        } else {
            stat = v1;
        }
    } else if (nc == 2) {
        stat = v1;   /* URC: <stat>,<lac>,<ci> */
    } else {
        if (c1) {
            parse_int(c1 + 1, &stat);
        }
    }

    fe_send_gprs_reg(stat);
}

static void handle_cusd(const char *line)
{
    const char *p = line + 6;
    int code;
    char text[PHONE_JSON_MAX / 2];

    parse_int(p, &code);
    extract_quoted(p, text, sizeof(text));
    fe_send_ussd_resp(code, text);
}

static void handle_cops(const char *line)
{
    char name[64];
    extract_quoted(line + 6, name, sizeof(name));
    fe_send_net_opr(name);
}

void handle_cpin(const char *line)
{
    const char *p = line + 6;
    while (*p == ' ') p++;
    strncpy(g_sim_state, p, sizeof(g_sim_state) - 1);
    g_sim_state[sizeof(g_sim_state) - 1] = '\0';
    fe_send_sim_stat(p);
}

static void handle_cbc(const char *line)
{
    (void)line;
    g_bcl = 100;
    fe_send_battery(0, 100);
}

static void handle_imei(const char *line)
{
    if (line[0] == '\0') return;
    fe_send_imei(line);
}

static void handle_ctzv(const char *line)
{
    /* +CTZV: 28,26/06/11,09:19:47 — no quotes; first token is the tz offset */
    char tz[16] = "";
    const char *p = line + 6;
    int i = 0;

    while (*p == ' ') {
        p++;
    }
    while (*p && *p != ',' && i < (int)sizeof(tz) - 1) {
        tz[i++] = *p++;
    }
    tz[i] = '\0';
    fe_send_timezone(tz, -1);
}

static void handle_ctze(const char *line)
{
    char tz[16];
    int dst = 0;
    const char *p;

    extract_quoted(line + 6, tz, sizeof(tz));
    p = strchr(line + 6, ',');
    if (p) {
        parse_int(p + 1, &dst);
    }
    fe_send_timezone(tz, dst);
}

static void handle_cclk(const char *line)
{
    char ts[32];

    extract_quoted(line + 6, ts, sizeof(ts));
    fe_send_clock(ts);
}

static void handle_cfun(const char *line)
{
    int fun = 0;

    parse_int(line + 6, &fun);
    fe_send_radio_state(fun);
}

static void handle_cpbr(const char *line)
{
    int index = 0;
    int type = 0;
    char num[CALLER_NUM_MAX];
    char name[PB_NAME_MAX];
    const char *p = line + 6;

    parse_int(p, &index);
    p = strchr(p, ',');
    if (!p) {
        return;
    }
    extract_quoted(p + 1, num, sizeof(num));
    p = strchr(p + 1, ',');
    if (!p) {
        fe_send_pb_entry(index, num, "", 0);
        return;
    }
    p = strchr(p + 1, ',');
    parse_int(p ? p - 4 : line, &type);
    name[0] = '\0';
    if (p) {
        extract_quoted(p, name, sizeof(name));
    }
    fe_send_pb_entry(index, num, name, type);
}

static void handle_iccid(const char *line)
{
    const char *p = strchr(line, ':');

    if (!p) {
        return;
    }
    p++;
    while (*p == ' ') {
        p++;
    }
    fe_send_iccid(p);
}

static void handle_cnum(const char *line)
{
    char num[CALLER_NUM_MAX];
    const char *p = strchr(line + 6, ',');

    if (!p) {
        return;
    }
    extract_quoted(p + 1, num, sizeof(num));
    fe_send_phone_num(num);
}

/* +CGPS: <mode>,<status>  (query response) */
static void handle_cgps(const char *line)
{
    int mode = 0;
    int status = 0;
    const char *p = line + 6;

    parse_int(p, &mode);
    p = strchr(p, ',');
    if (p) {
        parse_int(p + 1, &status);
    }
    g_gps_on = (mode >= 1 && status == 1) ? 1 : 0;
    rpi_send_gps_state(g_gps_on);
}

/* +CGPSINFO: <lat>,<N/S>,<lon>,<E/W>,<date>,<utc>,<alt>,<speed>,<course> */
static void handle_cgpsinfo(const char *line)
{
    const char *p = line + 10;
    char f[9][20]; int fi, ci, i;
    int lat_1e6 = 0, lon_1e6 = 0, alt_dm = 0, speed_10 = 0, course_10 = 0, fix;

    for (i = 0; i < 9; i++) f[i][0] = '\0';
    while (*p == ' ') p++;
    for (fi = 0, ci = 0; *p && fi < 9; p++) {
        if (*p == ',') { f[fi][ci] = '\0'; fi++; ci = 0; }
        else if (ci < 19) f[fi][ci++] = *p;
    }
    if (fi < 9) f[fi][ci] = '\0';

    /* f[0]=lat f[1]=N/S f[2]=lon f[3]=E/W f[4]=date f[5]=utc f[6]=alt f[7]=spd f[8]=crs */
    fix = (f[0][0] != '\0' && f[1][0] != '\0');
    if (fix) {
        if (parse_nmea_coord_1e6(f[0], f[1][0], &lat_1e6) < 0 ||
            parse_nmea_coord_1e6(f[2], f[3][0], &lon_1e6) < 0) {
            fix = 0;
        }
    }
    if (fix) {
        alt_dm    = parse_fixed_1dec(f[6]);
        speed_10  = parse_fixed_1dec(f[7]);
        course_10 = parse_fixed_1dec(f[8]);
    }
    rpi_send_gps_loc(lat_1e6, lon_1e6, alt_dm, speed_10, course_10, fix);
}

/*
 * Classify + dispatch a SIM line that is a URC or a solicited response datum.
 * Returns 1 if handled here (do NOT feed to the AT machine), 0 otherwise.
 */
int dispatch_line(const char *line)
{
    /* Call lifecycle */
    if (strcmp(line, "RING") == 0)                 { handle_ring(); return 1; }
    if (strncmp(line, "+CLIP:", 6) == 0)           { handle_clip(line); return 1; }
    if (strncmp(line, "+COLP:", 6) == 0)           { handle_colp(); return 1; }
    if (strcmp(line, "CONNECT") == 0 ||
        strncmp(line, "CONNECT ", 8) == 0 ||
        strcmp(line, "VOICE CALL: BEGIN") == 0)    { handle_voice_call_begin(); return 1; }
    if (strncmp(line, "VOICE CALL: END:", 16) == 0){ handle_voice_call_end(line + 16); return 1; }
    if (strcmp(line, "NO CARRIER") == 0 ||
        strcmp(line, "BUSY") == 0 ||
        strcmp(line, "NO ANSWER") == 0 ||
        strcmp(line, "NO DIALTONE") == 0)          { handle_call_release(line); return 1; }
    if (strncmp(line, "MISSED_CALL:", 12) == 0)    { handle_call_release(line); return 1; }
    if (strncmp(line, "+CLCC:", 6) == 0)           { handle_clcc(line); return 1; }
    if (strncmp(line, "+CCWA:", 6) == 0)           { handle_ccwa(line); return 1; }
    if (strncmp(line, "+CEER:", 6) == 0)           { handle_ceer(line); return 1; }
    /* SMS delivery + storage notification */
    if (strncmp(line, "+CMT:", 5) == 0)            { handle_cmt_header(line); return 1; }
    if (strncmp(line, "+CMTI:", 6) == 0)           { handle_cmti(line); return 1; }
    /* Network / status */
    if (strncmp(line, "+CSQ:", 5) == 0)            { handle_csq(line); return 1; }
    if (strncmp(line, "+CREG:", 6) == 0)           { handle_creg(line); return 1; }
    if (strncmp(line, "+CGREG:", 7) == 0)          { handle_cgreg(line); return 1; }
    /* SIM */
    if (strncmp(line, "+CPIN:", 6) == 0)           { handle_cpin(line); return 1; }
    return 0;   /* not a URC — pass to AT response consumer */
}

/* Top-level handler for one assembled SIM line. */
void sim_line_handler(const char *line)
{
    /* Suppress routine heartbeat to keep the console quiet between events. */
    if (!(at_busy && at_cur_kind == STEP_HEARTBEAT))
        printf("[pd] sim< %s\n", line);
    if (g_body_pending != BODY_NONE) {
        /* Guard: OK/ERROR — flush any accumulated body, then let AT machine handle. */
        if (strcmp(line, "OK") == 0 || is_final_error(line)) {
            printf("[pd] body-pending but got '%s' — flushing then routing\n", line);
            sms_body_hex_flush();
            g_body_pending = BODY_NONE;
            at_consume(line);
            return;
        }
        /* Guard: new +CMT arrives before previous body terminated — flush first. */
        if (strncmp(line, "+CMT:", 5) == 0) {
            sms_body_hex_flush();
            g_body_pending = BODY_NONE;
            handle_cmt_header(line);
            return;
        }
        /* BODY_CMT: accumulate hex-only lines; flush on first non-hex line.
         * SIM7600CE wraps long UCS2 hex bodies mid-sequence — each piece is a
         * valid all-hex line.  Accumulate until the idle timer fires or a
         * non-hex / non-CMT line arrives (signals end-of-body). */
        if (g_body_pending == BODY_CMT) {
            if (is_all_hex_line(line)) {
                int len = (int)strlen(line);
                if (g_body_hex_acc_len + len < SMS_HEX_MAX) {
                    memcpy(g_body_hex_acc + g_body_hex_acc_len, line, (size_t)len);
                    g_body_hex_acc_len += len;
                }
                g_body_hex_acc_len += len;
                g_body_hex_flush_ms = pd_now_ms() + BODY_HEX_FLUSH_MS;
                return;
            }
            /* Non-hex BODY_CMT line (plain-text SMS body — not UCS2 hex): flush
             * any previously accumulated hex (shouldn't mix), then pass line as body. */
            if (g_body_hex_acc_len > 0) {
                sms_body_hex_flush();
            }
            handle_sms_body(line);
            return;
        }
        /* BODY_CMGL: always single-line text mode body. */
        handle_sms_body(line);
        return;
    }
    if (dispatch_line(line)) {
        return;
    }
    at_consume(line);   /* OK / ERROR / +CME / +CMS / +CMGS or ignored */
}

/* SECTION 7 — non-blocking SIM line assembler + '>' prompt */

#define SIM_LA_CAP  2048	/* max length of one SIM line (static; no heap) */
static char sim_la[SIM_LA_CAP];
static int  sim_la_len = 0;

void sim_la_reset(void)
{
    sim_la_len = 0;
    sim_la[0]  = '\0';
}

void sim_feed(const unsigned char *buf, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        unsigned char ch = buf[i];
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            if (sim_la_len > 0) {
                while (sim_la_len > 0 && sim_la[sim_la_len - 1] == ' ') {
                    sim_la_len--;
                }
                if (sim_la_len > 0) {
                    sim_la[sim_la_len] = '\0';
                    sim_line_handler(sim_la);
                }
            }
            sim_la_len = 0;
            continue;
        }
        /* Discard stray '>' when not expecting the SMS prompt — otherwise it
         * prepends the next real response line and breaks AT response matching.
         * Also drop a leading space that the modem sends after "> ". */
        if (sim_la_len == 0 && ch == '>' &&
            !(at_busy && at_wait == WAIT_PROMPT)) {
            printf("[pd] discarding stray '>'\n");
            continue;
        }
        if (ch == ' ' && sim_la_len == 0) {
            continue;
        }
        /* drop the char if the line overflows the fixed buffer */
        if (sim_la_len + 2 > SIM_LA_CAP) {
            continue;
        }
        sim_la[sim_la_len++] = (char)ch;
    }
    /* Bare '>' prompt (no newline): only meaningful while awaiting it. */
    if (at_busy && at_wait == WAIT_PROMPT && sim_la_len >= 1 && sim_la[0] == '>') {
        sim_la_len = 0;
        if (at_cur_kind == STEP_SMS_HDR) {
            printf("[pd] sim< <PROMPT>\n");
        }
        at_finish(1);   /* prompt satisfied → next step writes the body */
    }
}

void sim_read_chunk(void)
{
    unsigned char buf[READ_CHUNK];
    ssize_t n;

    if (fd_sim < 0) {
        return;
    }
    for (;;) {
        n = read(fd_sim, buf, sizeof(buf));
        if (n > 0) {
            sim_feed(buf, (int)n);
            if (n < (ssize_t)sizeof(buf)) {
                break;
            }
            continue;
        }
        break;   /* nothan_v2: read returns -1 when the RX ring is empty (no data).
                  * A dead modem is detected by the AT heartbeat, not a read error. */
    }
}

/* SECTION 8 — non-blocking RPi4 frame assembler */

static unsigned char fe_buf[PHONE_FRAME_MAX * 2];
static int           fe_buf_len = 0;

void dispatch_cmd(const char *json);   /* fwd */

void fe_parse(void)
{
    for (;;) {
        int i, magic = -1, len;
        for (i = 0; i + 1 < fe_buf_len; i++) {
            if (fe_buf[i] == PHONE_MAGIC0 && fe_buf[i + 1] == PHONE_MAGIC1) {
                magic = i;
                break;
            }
        }
        if (magic < 0) {
            if (fe_buf_len > 0) {
                fe_buf[0] = fe_buf[fe_buf_len - 1];
                fe_buf_len = 1;
            }
            return;
        }
        if (magic > 0) {
            memmove(fe_buf, fe_buf + magic, (size_t)(fe_buf_len - magic));
            fe_buf_len -= magic;
        }
        if (fe_buf_len < 4) {
            return;
        }
        len = fe_buf[2] | (fe_buf[3] << 8);
        if (len == 0 || len >= PHONE_JSON_MAX) {
            memmove(fe_buf, fe_buf + 2, (size_t)(fe_buf_len - 2));
            fe_buf_len -= 2;
            continue;
        }
        if (fe_buf_len < len + 6) {
            return;
        }
        {
            uint16_t rx_crc = fe_buf[4 + len] | (fe_buf[5 + len] << 8);
            uint16_t calc    = phone_crc16(fe_buf + 4, (size_t)len);
            char json[PHONE_JSON_MAX];
            if (rx_crc == calc) {
                memcpy(json, fe_buf + 4, (size_t)len);
                json[len] = '\0';
                printf("[pd] fe< %s\n", json);
                dispatch_cmd(json);
            } else {
                printf("[pd] fe< CRC mismatch (got %04x want %04x)\n", rx_crc, calc);
            }
            memmove(fe_buf, fe_buf + len + 6, fe_buf_len - (len + 6));
            fe_buf_len -= (len + 6);
        }
    }
}

void fe_read_chunk(void)
{
    ssize_t n;

    if (fd_fe < 0) {
        return;
    }
    for (;;) {
        if (fe_buf_len >= (int)sizeof(fe_buf)) {
            fe_buf_len = 0;
        }  /* runaway guard */
        n = read(fd_fe, fe_buf + fe_buf_len, sizeof(fe_buf) - (size_t)fe_buf_len);
        if (n > 0) {
            fe_buf_len += (int)n;
            fe_parse();
            if (n < (ssize_t)(sizeof(fe_buf) - (size_t)fe_buf_len)) {
                break;
            }
            continue;
        }
        if (n == 0) {
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            break;
        }
        break;   /* RPi link error is non-fatal; FE will reconnect + HELLO */
    }
}

/* SECTION 9 — RPi4 command handlers */

void handle_cmd_dial(const char *json)
{
    char num[CALLER_NUM_MAX];
    char cmd[CALLER_NUM_MAX + 8];

    if (json_get_str(json, "num", num, sizeof(num)) < 0) {
        return;
    }
    if (cs.in_call || cs.outgoing) {
        printf("[pd] REJECT dial: already in call (in_call=%d outgoing=%d)\n",
               cs.in_call, cs.outgoing);
        fe_send_err(409, "call already active");
        return;
    }
    if (at_free_slots() < 1) {
        printf("[pd] REJECT dial: at queue full (%d used)\n", at_count);
        fe_send_err(503, "at queue full");
        return;
    }
    printf("[pd] dialing %s\n", num);
    cs.outgoing = 1;
    strncpy(cs.caller_num, num, sizeof(cs.caller_num) - 1);
    cs.caller_num[sizeof(cs.caller_num) - 1] = '\0';
    snprintf(cmd, sizeof(cmd), "ATD%s;\r", num);
    at_enqueue(cmd, -1, WAIT_FINAL, 60000, STEP_DIAL, -1);
}

void handle_cmd_answer(void)
{
    at_submit("ATA\r", 10000);
}

/* End or reject the current call.
 *
 * Normal case (active call, or incoming RING): queue AT+CHUP.
 *
 * Outgoing call still in setup (alerting, not yet answered): the ATD step is
 * holding the AT queue with WAIT_FINAL — its final OK does not arrive until
 * the call connects — so a queued AT+CHUP would never be written, which is why
 * hanging up only worked after the call connected. The modem stays in command
 * mode while the call alerts (a bare CR is ignored, not treated as an abort),
 * so we write AT+CHUP straight to it, bypassing the blocked queue, then tear
 * down the stuck ATD step locally so the queue unblocks and release call state.
 * The VOICE CALL: END / NO CARRIER URC then lands in handle_call_release() as a
 * no-op (state already cleared). */
void call_terminate(int reject)
{
    cs.local_hangup = 1;
    if (reject) {
        cs.rejected = 1;
    }

    if (at_busy && at_cur_kind == STEP_DIAL) {
        printf("[pd] terminate during dial setup — CHUP + abort in-flight ATD\n");
        sim_raw_write("AT+CHUP\r", 8);          /* disconnect the alerting MO call */
        at_busy = 0; at_wait = WAIT_NONE;
        at_cur_kind = STEP_PLAIN; at_cur_ctx = -1;
        handle_call_release(reject ? "REJECTED" : "LOCAL HANGUP");
        at_pump();
        return;
    }
    at_enqueue("AT+CHUP\r", -1, WAIT_FINAL, 5000, STEP_HANGUP, -1);
}

void handle_cmd_hangup(void)
{
    call_terminate(0);
}
void handle_cmd_reject(void)
{
    call_terminate(1);
}

static void handle_cmd_hold(void)
{
    at_submit("AT+CHLD=2\r", 5000);
}
static void handle_cmd_conference(void)
{
    at_submit("AT+CHLD=3\r", 5000);
}
static void handle_cmd_transfer(void)
{
    at_submit("AT+CHLD=4\r", 5000);
}
static void handle_cmd_release_held(void)
{
    at_submit("AT+CHLD=1\r", 5000);
}

static void handle_cmd_dtmf(const char *json)
{
    char tone[DTMF_TONE_MAX];
    char cmd[16];

    if (json_get_str(json, "tone", tone, sizeof(tone)) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+VTS=%s\r", tone);
    at_submit(cmd, 3000);
}

static void clamp_send(const char *at, int level, int min, int max)
{
    char cmd[32];

    if (level < min) {
        level = min;
    }
    if (level > max) {
        level = max;
    }
    snprintf(cmd, sizeof(cmd), "%s%d\r", at, level);
    at_submit(cmd, 3000);
}

static void handle_cmd_vol(const char *json)
{
    int level;

    if (json_get_int(json, "level", &level) < 0) {
        return;
    }
    clamp_send("AT+CLVL=", level, 0, VOL_LEVEL_MAX);
}
static void handle_cmd_ring_vol(const char *json)
{
    int level;

    if (json_get_int(json, "level", &level) < 0) {
        return;
    }
    clamp_send("AT+CRSL=", level, 0, VOL_LEVEL_MAX);
}
static void handle_cmd_mic_gain(const char *json)
{
    int level;

    if (json_get_int(json, "level", &level) < 0) {
        return;
    }
    clamp_send("AT+CMICGAIN=", level, 0, MIC_LEVEL_MAX);
}
static void handle_cmd_audio_route(const char *json)
{
    int dev;

    if (json_get_int(json, "dev", &dev) < 0) {
        return;
    }
    clamp_send("AT+CSDVC=", dev, 0, 3);
}

/* Build one PDU (single or one part of a concat series) for UCS2 text.
 * part_hex: exactly nchars*4 hex chars from body_hex.
 * Returns TPDU byte count for AT+CMGS=<n>, or -1 on error.
 * out must be >= (20 + nchars*2)*2 + 1 bytes (≈ 320 for 67-char part). */
static int sms_build_pdu_ucs2(const char *num,
                               const char *part_hex, int nchars,
                               int multi, unsigned char ref,
                               int nparts, int pnum,
                               char *out, int outsz)
{
    static const char H[] = "0123456789ABCDEF";
    unsigned char b[32];
    int o = 0, i;
    const char *dig;
    int ton_npi, da_n, udl;

    if (*num == '+') {
        dig = num + 1;
        ton_npi = 0x91;
    } else {
        dig = num;
        ton_npi = 0x81;
    }
    for (da_n = 0; dig[da_n] >= '0' && dig[da_n] <= '9'; da_n++) {
        /* count digits */
    }
    if (da_n == 0 || da_n > 20) {
        return -1;
    }

    udl = (multi ? 6 : 0) + nchars * 2;

    b[o++] = 0x00;                                  /* SMSC: use default     */
    b[o++] = (unsigned char)(multi ? 0x51 : 0x11);  /* SUBMIT|VP|[UDHI]      */
    b[o++] = 0x00;                                  /* MR                    */
    b[o++] = (unsigned char)da_n;
    b[o++] = (unsigned char)ton_npi;
    for (i = 0; i < da_n; i += 2) {
        unsigned char lo = (unsigned char)(dig[i]     - '0');
        unsigned char hi = (i + 1 < da_n) ? (unsigned char)(dig[i+1] - '0') : 0x0F;
        b[o++] = (unsigned char)((hi << 4) | lo);
    }
    b[o++] = 0x00;                  /* PID          */
    b[o++] = 0x08;                  /* DCS = UCS2   */
    b[o++] = 0xAA;                  /* VP ≈ 4 days  */
    b[o++] = (unsigned char)udl;
    if (multi) {
        b[o++] = 0x05; b[o++] = 0x00; b[o++] = 0x03;
        b[o++] = ref;
        b[o++] = (unsigned char)nparts;
        b[o++] = (unsigned char)pnum;
    }

    if (o * 2 + nchars * 4 + 1 > outsz) {
        return -1;
    }

    int pos = 0;
    for (i = 0; i < o; i++) {
        out[pos++] = H[(b[i] >> 4) & 0xF];
        out[pos++] = H[b[i] & 0xF];
    }
    memcpy(out + pos, part_hex, (size_t)(nchars * 4));
    out[pos + nchars * 4] = '\0';
    return (o - 1) + nchars * 2;   /* TPDU byte count for AT+CMGS=<n> */
}

/* Drop all consecutive STEP_SMS_HDR / STEP_SMS_RESULT from queue head.
 * Stops at first non-SMS step (e.g. STEP_SMS_MODE_RESTORE / STEP_PLAIN).
 * Safe for single SMS: no-op if no more SMS steps remain. */
void at_cancel_queued_sms(void)
{
    while (at_count > 0) {
        int k = at_q[at_head].kind;
        if (k != STEP_SMS_HDR && k != STEP_SMS_RESULT) {
            break;
        }
        printf("[pd] cancel queued sms step kind=%d\n", k);
        at_head = (at_head + 1) % AT_QUEUE_MAX;
        at_count--;
    }
}

/* Returns 1 if more STEP_SMS_RESULT are queued (i.e. this is an intermediate part). */
int at_has_more_sms_result(void)
{
    int i;
    for (i = 0; i < at_count; i++) {
        if (at_q[(at_head + i) % AT_QUEUE_MAX].kind == STEP_SMS_RESULT) {
            return 1;
        }
    }
    return 0;
}

int at_has_sms_pending(void)
{
    int i;
    if (at_busy && (at_cur_kind == STEP_SMS_HDR || at_cur_kind == STEP_SMS_RESULT
                    || at_cur_kind == STEP_SMS_MODE_RESTORE))
        return 1;
    for (i = 0; i < at_count; i++) {
        int k = at_q[(at_head + i) % AT_QUEUE_MAX].kind;
        if (k == STEP_SMS_HDR || k == STEP_SMS_RESULT || k == STEP_SMS_MODE_RESTORE)
            return 1;
    }
    return 0;
}

void handle_cmd_sms(const char *json)
{
    char num[CALLER_NUM_MAX], text[SMS_TEXT_MAX];
    int  cid = -1;
    if (json_get_str(json, "num",  num,  sizeof(num))  < 0 ||
        json_get_str(json, "text", text, sizeof(text)) < 0) {
        printf("[pd] bad CMD_SMS json\n");
        return;
    }
    json_get_int(json, "cid", &cid);

    if (at_has_sms_pending()) {
        printf("[pd] REJECT cid=%d: sms already in flight\n", cid);
        fe_send_sms_err(503, "sms busy", cid);
        return;
    }

    if (cs.ucs2_mode) {
        char body_hex[SMS_HEX_MAX + 1];
        if (utf8_to_ucs2_hex(text, body_hex, sizeof(body_hex)) < 0) {
            fe_send_sms_err(-1, "ucs2 encode text", cid);
            return;
        }
        int total_chars = (int)(strlen(body_hex) / 4);

        if (total_chars <= SMS_UCS2_SINGLE_MAX) {
            /* ── single part: text mode ── */
            char num_hex[CALLER_NUM_MAX * 4 + 1];
            char hdr[CALLER_NUM_MAX * 4 + 16];
            char body[SMS_HEX_MAX + 2];
            if (at_free_slots() < 2) {
                fe_send_sms_err(503, "at queue full", cid);
                return;
            }
            if (utf8_to_ucs2_hex(num, num_hex, sizeof(num_hex)) < 0) {
                fe_send_sms_err(-1, "ucs2 encode num", cid);
                return;
            }
            printf("[pd] cid=%d ucs2-single chars=%d num_hex=%s\n",
                   cid, total_chars, num_hex);
            snprintf(hdr, sizeof(hdr), "AT+CMGS=\"%s\"\r", num_hex);
            at_enqueue(hdr, -1, WAIT_PROMPT, 10000, STEP_SMS_HDR, cid);
            snprintf(body, sizeof(body), "%s%c", body_hex, 0x1A);
            at_enqueue(body, -1, WAIT_SMS_RESULT, 25000, STEP_SMS_RESULT, cid);
        } else {
            /* ── multipart: PDU mode with UDH ── */
            int p, nparts = (total_chars + SMS_UCS2_PART_CHARS - 1) / SMS_UCS2_PART_CHARS;
            if (nparts > SMS_CONCAT_PARTS_MAX) {
                fe_send_sms_err(-1, "message too long", cid);
                return;
            }
            /* slots: 1(CMGF=0) + 2*nparts + 1(CMGF=1) + 1(CSCS) */
            if (at_free_slots() < 2 * nparts + 3) {
                fe_send_sms_err(503, "at queue full", cid);
                return;
            }
            unsigned char ref = g_sms_concat_ref++;
            if (g_sms_concat_ref == 0) {
                g_sms_concat_ref = 1;
            }
            printf("[pd] cid=%d ucs2-multi chars=%d nparts=%d ref=%d\n",
                   cid, total_chars, nparts, (int)ref);

            at_submit("AT+CMGF=0\r", 3000);

            for (p = 0; p < nparts; p++) {
                int start = p * SMS_UCS2_PART_CHARS;
                int nc    = total_chars - start;
                char pdu_hex[320], hdr[28], body[322];
                int tlen;
                if (nc > SMS_UCS2_PART_CHARS) nc = SMS_UCS2_PART_CHARS;
                tlen = sms_build_pdu_ucs2(num, body_hex + start * 4, nc,
                                          1, ref, nparts, p + 1,
                                          pdu_hex, sizeof(pdu_hex));
                if (tlen < 0) {
                    at_cancel_queued_sms();
                    at_enqueue("AT+CMGF=1\r", -1, WAIT_FINAL, 3000,
                               STEP_SMS_MODE_RESTORE, -1);
                    at_submit("AT+CSCS=\"UCS2\"\r", 3000);
                    fe_send_sms_err(-1, "pdu build", cid);
                    return;
                }
                snprintf(hdr, sizeof(hdr), "AT+CMGS=%d\r", tlen);
                at_enqueue(hdr, -1, WAIT_PROMPT, 10000, STEP_SMS_HDR, cid);
                snprintf(body, sizeof(body), "%s%c", pdu_hex, 0x1A);
                at_enqueue(body, (int)strlen(pdu_hex) + 1,
                           WAIT_SMS_RESULT, 25000, STEP_SMS_RESULT, cid);
            }
            /* Restore text mode + UCS2 charset after all parts */
            at_enqueue("AT+CMGF=1\r", -1, WAIT_FINAL, 3000,
                       STEP_SMS_MODE_RESTORE, -1);
            at_submit("AT+CSCS=\"UCS2\"\r", 3000);
        }
    } else {
        /* ── GSM7 text mode ── */
        char hdr[CALLER_NUM_MAX + 16];
        char body[SMS_TEXT_MAX + 2];

        if (at_free_slots() < 2) {
            fe_send_sms_err(503, "at queue full", cid);
            return;
        }
        printf("[pd] cid=%d gsm num=%s len=%d\n", cid, num, (int)strlen(text));
        snprintf(hdr, sizeof(hdr), "AT+CMGS=\"%s\"\r", num);
        at_enqueue(hdr, -1, WAIT_PROMPT, 10000, STEP_SMS_HDR, cid);
        snprintf(body, sizeof(body), "%s%c", text, 0x1A);
        at_enqueue(body, -1, WAIT_SMS_RESULT, 25000, STEP_SMS_RESULT, cid);
    }
}



static void handle_cmd_sms_list(const char *json)
{
    char stat[16] = "ALL";
    char cmd[40];

    if (cs.outgoing || cs.in_call || cs.pending_clip) {
        fe_send_err(409, "busy: call in progress");
        return;
    }
    if (json) {
        json_get_str(json, "stat", stat, sizeof(stat));
    }
    snprintf(cmd, sizeof(cmd), "AT+CMGL=\"%s\"\r", stat);
    at_enqueue(cmd, -1, WAIT_FINAL, 30000, STEP_SMS_LIST, -1);
}

static void handle_cmd_sms_read(const char *json)
{
    int index;
    char cmd[32];

    if (json_get_int(json, "index", &index) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+CMGR=%d\r", index);
    at_submit(cmd, 10000);
}

static void handle_cmd_sms_delete(const char *json)
{
    int index = -1; char cmd[32];
    if (json && json_get_int(json, "index", &index) == 0 && index >= 0) {
        snprintf(cmd, sizeof(cmd), "AT+CMGD=%d\r", index);
        at_submit(cmd, 5000);
    } else {
        at_submit("AT+CMGD=1,4\r", 10000);
    }
}

static void handle_cmd_sms_ucs2(const char *json)
{
    int on = 0;

    if (json) {
        json_get_int(json, "on", &on);
    }
    cs.ucs2_mode = on ? 1 : 0;
    if (on) {
        at_submit("AT+CSCS=\"UCS2\"\r", 3000);
        at_submit("AT+CSMP=17,167,0,8\r", 3000);   /* DCS=8 → UCS2 */
    } else {
        at_submit("AT+CSCS=\"GSM\"\r", 3000);
        at_submit("AT+CSMP=17,167,0,0\r", 3000);   /* DCS=0 → GSM7 */
    }
}

static void handle_cmd_net_select(const char *json)
{
    char mccmnc[16];
    char cmd[40];

    if (json_get_str(json, "mccmnc", mccmnc, sizeof(mccmnc)) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+COPS=1,2,\"%s\"\r", mccmnc);
    at_submit(cmd, 60000);
}

static void handle_cmd_ussd(const char *json)
{
    char code[USSD_CODE_MAX];
    char cmd[USSD_CODE_MAX + 24];

    if (json_get_str(json, "code", code, sizeof(code)) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+CUSD=1,\"%s\",15\r", code);
    at_submit(cmd, 30000);
}

static void handle_cmd_timezone(const char *json)
{
    int on = 1;

    if (json) {
        json_get_int(json, "on", &on);
    }
    at_submit(on ? "AT+CTZR=1\r" : "AT+CTZR=0\r", 3000);
}

static void handle_cmd_cf_set(const char *json)
{
    int reason = 0;
    int mode = 0;
    char num[CALLER_NUM_MAX] = "";
    char cmd[80];

    json_get_int(json, "reason", &reason);
    json_get_int(json, "mode",   &mode);
    json_get_str(json, "num",    num, sizeof(num));
    if (num[0]) {
        snprintf(cmd, sizeof(cmd), "AT+CCFC=%d,%d,\"%s\",129\r", reason, mode, num);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+CCFC=%d,%d\r", reason, mode);
    }
    at_submit(cmd, 10000);
}

static void handle_cmd_cf_query(const char *json)
{
    int reason = 0;
    char cmd[32];

    if (json) {
        json_get_int(json, "reason", &reason);
    }
    snprintf(cmd, sizeof(cmd), "AT+CCFC=%d,2\r", reason);
    at_submit(cmd, 10000);
}

static void handle_cmd_pb_read(const char *json)
{
    int start = 1;
    int end = 250;
    char cmd[32];

    if (json) {
        json_get_int(json, "start", &start);
        json_get_int(json, "end", &end);
    }
    at_submit("AT+CPBS=\"SM\"\r", 5000);
    snprintf(cmd, sizeof(cmd), "AT+CPBR=%d,%d\r", start, end);
    at_enqueue(cmd, -1, WAIT_FINAL, 15000, STEP_PB_READ, -1);
}

static void handle_cmd_pb_write(const char *json)
{
    char num[CALLER_NUM_MAX];
    char name[PB_NAME_MAX];
    char cmd[128];

    if (json_get_str(json, "num", num, sizeof(num)) < 0 ||
        json_get_str(json, "name", name, sizeof(name)) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+CPBW=,\"%s\",129,\"%s\"\r", num, name);
    at_submit(cmd, 5000);
}

static void handle_cmd_pb_delete(const char *json)
{
    int index;
    char cmd[32];

    if (json_get_int(json, "index", &index) < 0) {
        return;
    }
    snprintf(cmd, sizeof(cmd), "AT+CPBW=%d\r", index);
    at_submit(cmd, 5000);
}

static void handle_cmd_gps_start(void)
{
    at_submit("AT+CGPS=1,1\r", 5000);
    g_gps_on = 1;
    g_last_gps_poll = 0;
    rpi_send_gps_state(1);
}

static void handle_cmd_gps_stop(void)
{
    at_submit("AT+CGPS=0\r", 5000);
    g_gps_on = 0;
    rpi_send_gps_state(0);
}

/* HELLO handshake — FE sends this on connect; BE replays missed criticals and
 * sends a state snapshot. */
void handle_cmd_hello(const char *json)
{
    int fe_boot = 0;
    int last_seq = 0;

    json_get_int(json, "fe_boot",  &fe_boot);
    json_get_int(json, "last_seq", &last_seq);
    if (fe_boot && fe_boot != g_fe_boot) {
        g_fe_boot = fe_boot;
    }
    printf("[pd] HELLO fe_boot=%d last_seq=%d → replay+snapshot\n",
           fe_boot, last_seq);
    rel_replay_after(last_seq);
    send_ready_snapshot();
}

void dispatch_cmd(const char *json)
{
    char type[32];
    int  seq = -1;

    if (json_get_str(json, "type", type, sizeof(type)) < 0) {
        return;
    }

    if (strcmp(type, MSG_ACK) == 0) {
        json_get_int(json, "seq", &seq);
        rel_on_ack(seq);
        return;
    }

    if (strcmp(type, MSG_CMD_HELLO) == 0) {
        handle_cmd_hello(json);
        return;
    }

    /* Dedup reliable commands from FE by seq. */
    if (json_get_int(json, "seq", &seq) == 0 && seq > 0) {
        send_ack(seq);
        if (seq <= g_last_cmd_seq) {
            printf("[pd] dedup seq=%d type=%s\n", seq, type);
            return;
        }
        g_last_cmd_seq = seq;
    }

    /* ── reject modem actions while the modem is down/initializing, instead of
     *    silently dropping them (the AT queue is flushed on recovery). The FE
     *    gets a clear ERR/SMS_ERR and can retry once MODEM_UP arrives. ── */
    if (g_modem_state != MODEM_READY) {
        if (!strcmp(type, MSG_CMD_SMS)) {
            int cid = -1;
            json_get_int(json, "cid", &cid);
            fe_send_sms_err(503, "modem not ready", cid);
        } else {
            fe_send_err(503, "modem not ready");
        }
        return;
    }

    /* ── voice call ── */
    if (!strcmp(type, MSG_CMD_DIAL)) {
        handle_cmd_dial(json);
    } else if (!strcmp(type, MSG_CMD_ANSWER)) {
        handle_cmd_answer();
    } else if (!strcmp(type, MSG_CMD_HANGUP)) {
        handle_cmd_hangup();
    } else if (!strcmp(type, MSG_CMD_REJECT)) {
        handle_cmd_reject();
    } else if (!strcmp(type, MSG_CMD_CALL_END_ACK)) {
        /* no-op ack */
    } else if (!strcmp(type, MSG_CMD_MUTE)) {
        at_submit("AT+CMUT=1\r", 3000);
    } else if (!strcmp(type, MSG_CMD_UNMUTE)) {
        at_submit("AT+CMUT=0\r", 3000);
    }
    /* ── SMS ── */
    else if (!strcmp(type, MSG_CMD_SMS)) {
        handle_cmd_sms(json);
    } else {
        printf("[pd] unknown cmd: %s\n", type);
    }
}

/* SECTION 10 — init / recovery supervisor                            */

static int           g_was_recovering = 0;
static unsigned long g_reopen_at = 0;
static unsigned long g_reopen_backoff = REOPEN_BACKOFF_MS;
static unsigned long g_last_hb = 0;
static unsigned long g_last_beacon = 0;

void enqueue_init(void)
{
    at_submit("ATE0\r", 2000);
    at_submit("ATS0=0\r", 2000);
    at_submit("AT+CMEE=2\r", 2000);
    at_submit("AT+CMGF=1\r", 2000);
    at_submit("AT+CSMP=17,167,0,0\r", 2000);
    at_submit("AT+CSCS=\"GSM\"\r", 2000);
    at_submit("AT+CNMI=2,2,0,0,0\r", 2000);
    at_submit("AT+CLIP=1\r", 2000);
    at_submit("AT+CCWA=1,1\r", 2000);   /* call-waiting URC (keep so +CCWA fires for pending missed) */
    at_submit("AT+CREG=2\r", 2000);
    at_submit("AT+CGREG=2\r", 2000);
    at_submit("AT+CSDVC=1\r", 2000);
    at_submit("AT+CLVL=5\r", 2000);
    at_submit("AT+CMICGAIN=8\r", 2000);
    cs.ucs2_mode = 0;
}

void enqueue_state_queries(void)
{
    at_submit("AT+CPIN?\r", 3000);
    at_submit("AT+CSQ\r", 3000);
    at_submit("AT+CREG?\r", 3000);
}

void trigger_recover(const char *why)
{
    if (g_modem_state == MODEM_RECOVER) {
        return;
    }
    printf("[pd] modem down (%s) — reopening %s\n", why ? why : "?", SIM_DEV);
    fe_send_modem_down();
    g_modem_state = MODEM_RECOVER;
    g_was_recovering = 1;
    g_hb_fail = 0;
    /* abort any in-flight/queued AT work and reset the line assembler */
    at_busy = 0; at_wait = WAIT_NONE; at_head = at_tail = at_count = 0;
    sim_la_reset(); g_body_pending = BODY_NONE;
    if (fd_sim >= 0) { close(fd_sim); fd_sim = -1; }
    g_reopen_backoff = REOPEN_BACKOFF_MS;
    g_reopen_at = pd_now_ms() + g_reopen_backoff;
}

void recovery_tick(unsigned long now)
{
    if (g_modem_state != MODEM_RECOVER) {
        return;
    }
    if (now < g_reopen_at) {
        return;
    }

    fd_sim = open_uart(SIM_DEV);
    if (fd_sim < 0) {
        g_reopen_backoff *= 2;
        if (g_reopen_backoff > REOPEN_BACKOFF_MAX) {
            g_reopen_backoff = REOPEN_BACKOFF_MAX;
        }
        g_reopen_at = now + g_reopen_backoff;
        return;
    }
    printf("[pd] %s reopened — re-initializing modem\n", SIM_DEV);
    sim_la_reset();
    enqueue_init();
    g_modem_state = MODEM_INITIALIZING;
}

/* Detect the INITIALIZING→READY transition (queue drained, modem replied). */
void check_init_done(void)
{
    if (g_modem_state != MODEM_INITIALIZING) {
        return;
    }
    if (at_busy || at_count > 0) {
        return;
    }
    g_modem_state = MODEM_READY;
    printf("[pd] modem ready\n");
    enqueue_state_queries();
    if (g_was_recovering) {
        g_was_recovering = 0;
        fe_send_modem_up();
    }
    send_ready_snapshot();
    g_last_hb = pd_now_ms();
    g_last_beacon = g_last_hb;
}

void heartbeat_tick(unsigned long now)
{
    if (g_modem_state != MODEM_READY) {
        return;
    }
    if (now - g_last_hb < HB_INTERVAL_MS) {
        return;
    }
    if (at_busy || at_count > 0) {
        g_last_hb = now;
        return;
    }   /* link busy = alive */
    g_last_hb = now;
    at_enqueue("AT\r", -1, WAIT_FINAL, HB_TIMEOUT_MS, STEP_HEARTBEAT, -1);
}

void beacon_tick(unsigned long now)
{
    if (now - g_last_beacon < READY_BEACON_MS) {
        return;
    }
    g_last_beacon = now;
    send_ready_snapshot();   /* late/restarted FE re-HELLOs off this */
}

static void gps_poll_tick(unsigned long now)
{
    if (!g_gps_on) {
        return;
    }
    if (g_modem_state != MODEM_READY) {
        return;
    }
    if (now - g_last_gps_poll < GPS_POLL_MS) {
        return;
    }
    if (at_busy || at_count > 2) {
        return;   /* don't pile up behind a busy queue */
    }
    g_last_gps_poll = now;
    at_submit("AT+CGPSINFO\r", 3000);
}

/* SECTION 11 — multiplexer + super loop */

static void pd_wait(int timeout_ms)
{
#if PD_HAVE_POLL
    struct pollfd pfd[2];
    int n = 0;
    if (fd_sim >= 0) {
        pfd[n].fd = fd_sim;
        pfd[n].events = POLLIN;
        n++;
    }
    if (fd_fe >= 0) {
        pfd[n].fd = fd_fe;
        pfd[n].events = POLLIN;
        n++;
    }
    if (n == 0) {
        pd_sleep_us((unsigned)timeout_ms * 1000);
        return;
    }
    poll(pfd, (nfds_t)n, timeout_ms);
#else
    (void)timeout_ms;
    yield();   /* cooperative scheduling: give the GUI/shell a turn each loop */
#endif
}

static void super_loop(void)
{
    for (;;) {
        unsigned long now;
        pd_wait(POLL_TIMEOUT_MS);

        sim_read_chunk();
        fe_read_chunk();

        at_pump();

        now = pd_now_ms();
        at_timers_tick(now);
        clip_timer_tick(now);
        /* Flush SMS hex body accumulation after idle period (no more modem chunks). */
        if (g_body_pending == BODY_CMT && g_body_hex_flush_ms > 0
                && now >= g_body_hex_flush_ms) {
            sms_body_hex_flush();
        }
        sms_ci_tick(now);
        rel_retransmit_tick(now);
        recovery_tick(now);
        check_init_done();
        heartbeat_tick(now);
        beacon_tick(now);
        gps_poll_tick(now);
    }
}

/* SECTION 12 — entry point */

int main(void)
{
    printf("[pd] starting (single-owner)\n");

    memset(&cs, 0, sizeof(cs));
    cs.last_cause = -1;

    fd_sim = open_uart(SIM_DEV);
    if (fd_sim < 0) {
        printf("[pd] cannot open %s\n", SIM_DEV);
        return 1;
    }
    fd_fe = open_uart(RPI_DEV);
    if (fd_fe < 0) {
        printf("[pd] cannot open %s\n", RPI_DEV);
        close(fd_sim);
        return 1;
    }
    printf("[pd] sim=%s fe=%s opened\n", SIM_DEV, RPI_DEV);

    enqueue_init();
    g_modem_state = MODEM_INITIALIZING;

    super_loop();   /* never returns */
    return 0;
}
