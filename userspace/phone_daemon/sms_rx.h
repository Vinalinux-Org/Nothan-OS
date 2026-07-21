#ifndef SMS_RX_H
#define SMS_RX_H

/*
 * sms_rx — incoming multipart-SMS reassembly (text mode, no UDH).
 *
 * The modem in text mode (AT+CMGF=1) delivers each part of a long SMS as a
 * separate +CMT URC and hides the concatenation header (UDH), so we cannot
 * read the real part count / sequence / reference. Instead we exploit the one
 * fact the payload still carries: an SMSC packs every part EXCEPT the last to
 * the maximum size (67 UCS-2 code units, or 153 GSM-7 septets). So:
 *
 *   - a FULL part (== the max) → more parts are coming, keep collecting;
 *   - a SHORT part (< the max) → it is the final part (or a standalone), emit
 *     the whole buffer now.
 *
 * This joins the parts of one message regardless of inter-part delay, and —
 * crucially — keeps two SEPARATE short messages from the same sender apart
 * (each is short, so each emits on its own). A time window is kept only as a
 * safety net for the rare all-full message with no short tail, and to bound
 * how long a buffered sender lingers.
 *
 * The module is pure logic (no I/O, no globals from the daemon) so it can be
 * unit-tested on the host — see phone_daemon/test/sms_rx_test.c.
 */

/* Sink for a completed (possibly reassembled) message. In the daemon this is
 * fe_send_sms_in; in tests it is a capture. `ts` may be "". */
typedef void (*sms_rx_emit_fn)(const char *sender, const char *text,
			       const char *ts);

/* Set the emit sink. Call once at startup. */
void sms_rx_init(sms_rx_emit_fn emit);

/* Feed one decoded SMS part.
 *   sender    : originating address (already decoded to text)
 *   ts        : service-centre timestamp (may be "" / NULL)
 *   text      : the part's user text (already decoded + diacritic-stripped)
 *   part_chars: the part length in SMS characters BEFORE stripping — UCS-2
 *               code units (hexlen/4) or GSM-7 septets. Used to tell a full
 *               "more-coming" part from a short final/standalone part.
 *   is_ucs2   : 1 if the part was UCS-2 (67-char parts), else GSM-7 (153).
 *   now_ms    : monotonic clock in milliseconds.
 */
void sms_rx_part(const char *sender, const char *ts, const char *text,
		 int part_chars, int is_ucs2, unsigned long now_ms);

/* Flush any buffered sender whose safety window has expired. Call each loop. */
void sms_rx_tick(unsigned long now_ms);

#endif
