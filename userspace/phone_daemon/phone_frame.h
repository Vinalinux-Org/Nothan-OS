/*
 * phone_frame.h — frontend ↔ backend binary frame protocol (CANONICAL)
 *
 * Frame layout (all integers little-endian):
 *   [0xAA][0x55][LEN_LO][LEN_HI][JSON payload...][CRC_LO][CRC_HI]
 *
 * CRC16-CCITT (poly 0x1021, init 0xFFFF) over the JSON payload only.
 *
 * ──────────────────────────────────────────────────────────────────────────
 * RELIABILITY LAYER (seq only — no src_boot / no be_boot)
 * ──────────────────────────────────────────────────────────────────────────
 * Critical events (CALL_IN, SMS_IN, CALL_MISS, CALL_END, MODEM_DOWN, MODEM_UP)
 * carry one extra JSON field:
 *     "seq" : int — BE-side monotonic sequence number
 * The FE ACKs each with:
 *     {"type":"ACK","seq":<n>}
 * The BE retransmits an unACKed critical until the matching ACK arrives.
 * Receivers dedup by seq alone (no boot-id tracking needed).
 *
 * Handshake (startup-order independent):
 *   FE→BE  {"type":"CMD_HELLO","fe_boot":<id>,"last_seq":<n>}
 *   BE→FE  {"type":"READY","seq":<be_seq_high>,
 *           "call":"idle|ringing|active","callnum":"...",
 *           "sim":"READY","net":<stat>,"rssi":<n>,"bcl":<n>,"ucs2":<0|1>}
 *   The daemon emits a periodic READY beacon; if beacon.seq > FE.lastSeq the
 *   FE re-HELLOs so the daemon replays unACKed criticals with seq > last_seq.
 *
 * AT init (before main loop) — comprehensive SIM7600CE feature surface:
 *   ATE0, AT+CMEE=2, AT+CMGF=1, AT+CSCS="GSM", AT+CNMI=2,2,0,0,0,
 *   AT+CLIP=1, AT+COLP=1, AT+CCWA=1,1, AT+CSSN=1,1, AT+CREG=2, AT+CGREG=2,
 *   AT+CUSD=1, AT+CTZU=1, AT+CTZR=1, AT+CSDVC=1
 */

#ifndef PHONE_FRAME_H
#define PHONE_FRAME_H

#include "../lib/types.h"

/* Frame wire constants */
#define PHONE_MAGIC0       0xAA
#define PHONE_MAGIC1       0x55
#define PHONE_JSON_MAX     2048
#define PHONE_FRAME_MAX    (2 + 2 + PHONE_JSON_MAX + 2)  /* hdr+payload+crc */

/* ------------------------------------------------------------------ */
/* frontend event types                                              */
/* ------------------------------------------------------------------ */
#define MSG_READY         "READY"
#define MSG_ACK           "ACK"
#define MSG_ERR           "ERR"
#define MSG_CALL_IN       "CALL_IN"
#define MSG_CALL_ACT      "CALL_ACT"
#define MSG_CALL_END      "CALL_END"
#define MSG_CALL_MISS     "CALL_MISS"
#define MSG_CALL_STAT     "CALL_STAT"
#define MSG_CALL_WAIT     "CALL_WAIT"
#define MSG_CALL_HOLD     "CALL_HOLD"
#define MSG_CALL_PROG     "CALL_PROG"
#define MSG_CALL_RING     "CALL_RING"
#define MSG_CALL_DTMF_RX  "CALL_DTMF_RX"
#define MSG_SMS_IN        "SMS_IN"
#define MSG_SMS_STORED    "SMS_STORED"
#define MSG_SMS_DELIVER   "SMS_DELIVER"
#define MSG_SMS_ACK       "SMS_ACK"
#define MSG_SMS_ERR       "SMS_ERR"
#define MSG_SMS_LIST      "SMS_LIST"
#define MSG_SMS_LIST_END  "SMS_LIST_END"
#define MSG_SIGNAL        "SIGNAL"
#define MSG_NET_REG       "NET_REG"
#define MSG_GPRS_REG      "GPRS_REG"
#define MSG_NET_OPR       "NET_OPR"
#define MSG_NET_SCAN      "NET_SCAN"
#define MSG_NET_SCAN_END  "NET_SCAN_END"
#define MSG_USSD_RESP     "USSD_RESP"
#define MSG_TIMEZONE      "TIMEZONE"
#define MSG_CLOCK         "CLOCK"
#define MSG_SIM_STAT      "SIM_STAT"
#define MSG_BATTERY       "BATTERY"
#define MSG_IMEI          "IMEI"
#define MSG_ICCID         "ICCID"
#define MSG_PHONE_NUM     "PHONE_NUM"
#define MSG_RADIO_STATE   "RADIO_STATE"
#define MSG_AUDIO_ROUTE   "AUDIO_ROUTE"
#define MSG_PB_ENTRY      "PB_ENTRY"
#define MSG_PB_ENTRY_END  "PB_ENTRY_END"
#define MSG_SS_NOTIFY     "SS_NOTIFY"
#define MSG_CF_STATE      "CF_STATE"
#define MSG_MODEM_DOWN    "MODEM_DOWN"   /* modem unresponsive — recovery started */
#define MSG_MODEM_UP      "MODEM_UP"     /* modem re-initialized after recovery   */
#define MSG_GPS_STATE     "GPS_STATE"    /* GPS engine on/off: {"on":<0|1>}        */
#define MSG_GPS_LOC       "GPS_LOC"      /* fix data: lat,lon,alt,speed,course,hdop,sats,fix */

/* ------------------------------------------------------------------ */
/* frontend command types                                            */
/* ------------------------------------------------------------------ */
#define MSG_CMD_HELLO        "CMD_HELLO"   /* handshake / resync request          */
#define MSG_CMD_DIAL         "CMD_DIAL"
#define MSG_CMD_ANSWER       "CMD_ANSWER"
#define MSG_CMD_HANGUP       "CMD_HANGUP"
#define MSG_CMD_REJECT       "CMD_REJECT"
#define MSG_CMD_CALL_END_ACK "CMD_CALL_END_ACK"
#define MSG_CMD_DTMF         "CMD_DTMF"
#define MSG_CMD_HOLD         "CMD_HOLD"
#define MSG_CMD_RESUME       "CMD_RESUME"
#define MSG_CMD_SWAP         "CMD_SWAP"
#define MSG_CMD_CONFERENCE   "CMD_CONFERENCE"
#define MSG_CMD_TRANSFER     "CMD_TRANSFER"
#define MSG_CMD_RELEASE_HELD "CMD_RELEASE_HELD"
#define MSG_CMD_MUTE         "CMD_MUTE"
#define MSG_CMD_UNMUTE       "CMD_UNMUTE"
#define MSG_CMD_VOL          "CMD_VOL"
#define MSG_CMD_RING_VOL     "CMD_RING_VOL"
#define MSG_CMD_MIC_GAIN     "CMD_MIC_GAIN"
#define MSG_CMD_AUDIO_ROUTE  "CMD_AUDIO_ROUTE"
#define MSG_CMD_STATUS       "CMD_STATUS"
#define MSG_CMD_SMS          "CMD_SMS"
#define MSG_CMD_SMS_LIST     "CMD_SMS_LIST"
#define MSG_CMD_SMS_READ     "CMD_SMS_READ"
#define MSG_CMD_SMS_DELETE   "CMD_SMS_DELETE"
#define MSG_CMD_SMS_UCS2     "CMD_SMS_UCS2"
#define MSG_CMD_SIGNAL       "CMD_SIGNAL"
#define MSG_CMD_NET_OPR      "CMD_NET_OPR"
#define MSG_CMD_NET_SCAN     "CMD_NET_SCAN"
#define MSG_CMD_NET_SELECT   "CMD_NET_SELECT"
#define MSG_CMD_NET_AUTO     "CMD_NET_AUTO"
#define MSG_CMD_USSD         "CMD_USSD"
#define MSG_CMD_USSD_CANCEL  "CMD_USSD_CANCEL"
#define MSG_CMD_CLOCK        "CMD_CLOCK"
#define MSG_CMD_TIMEZONE     "CMD_TIMEZONE"
#define MSG_CMD_RADIO_ON     "CMD_RADIO_ON"
#define MSG_CMD_RADIO_OFF    "CMD_RADIO_OFF"
#define MSG_CMD_FLIGHT_MODE  "CMD_FLIGHT_MODE"
#define MSG_CMD_SIM_STAT     "CMD_SIM_STAT"
#define MSG_CMD_BATTERY      "CMD_BATTERY"
#define MSG_CMD_IMEI         "CMD_IMEI"
#define MSG_CMD_ICCID        "CMD_ICCID"
#define MSG_CMD_PHONE_NUM    "CMD_PHONE_NUM"
#define MSG_CMD_CF_SET       "CMD_CF_SET"
#define MSG_CMD_CF_QUERY     "CMD_CF_QUERY"
#define MSG_CMD_CW_ENABLE    "CMD_CW_ENABLE"
#define MSG_CMD_CW_DISABLE   "CMD_CW_DISABLE"
#define MSG_CMD_PB_READ      "CMD_PB_READ"
#define MSG_CMD_PB_WRITE     "CMD_PB_WRITE"
#define MSG_CMD_PB_DELETE    "CMD_PB_DELETE"
#define MSG_CMD_GPS_START    "CMD_GPS_START"  /* start GPS engine (AT+CGPS=1)          */
#define MSG_CMD_GPS_STOP     "CMD_GPS_STOP"   /* stop  GPS engine (AT+CGPS=0)          */
#define MSG_CMD_GPS_LOC      "CMD_GPS_LOC"    /* request one-shot location (AT+CGPSINFO) */

/* ------------------------------------------------------------------ */
/* API                                                                  */
/* ------------------------------------------------------------------ */

/* CRC16-CCITT (poly 0x1021, init 0xFFFF) over payload bytes. */
uint16_t phone_crc16(const uint8_t *data, size_t len);

/*
 * phone_frame_encode — pack json[json_len] into frame[frame_size].
 * Returns total frame byte count on success, -1 if buffer too small.
 */
int phone_frame_encode(uint8_t *frame, size_t frame_size,
                       const char *json, size_t json_len);

/*
 * phone_frame_decode — unpack frame[] into json[].
 * Returns JSON payload length on success, -1 on magic/CRC mismatch.
 */
int phone_frame_decode(const uint8_t *frame, size_t frame_size,
                       char *json, size_t json_size);

/*
 * phone_frame_read — read one complete frame from fd (blocks).
 * Resyncs on magic mismatch by scanning for next 0xAA byte.
 * Returns JSON length on success, -1 on I/O or CRC error.
 * NOTE: the canonical single-owner daemon uses a non-blocking frame
 * assembler instead (see phone_daemon.c); this remains for compat/tests.
 */
int phone_frame_read(int fd, char *json, size_t json_size);

/*
 * phone_frame_write — encode json as a frame and write atomically to fd.
 * Returns 0 on success, -1 on encode or write error.
 */
int phone_frame_write(int fd, const char *json, size_t json_len);

#endif /* PHONE_FRAME_H */
