/*
 * phone_daemon/test/sms_rx_test.c — host unit test for sms_rx reassembly.
 *
 * Build & run:
 *   gcc -g -O0 -Wall phone_daemon/sms_rx.c phone_daemon/test/sms_rx_test.c \
 *       -o /tmp/sms_rx_test && /tmp/sms_rx_test
 *
 * Drives sms_rx with synthetic +CMT part sequences and checks the emitted
 * (reassembled) messages — proving multipart join, separate-message split,
 * timing tolerance, GSM/UCS2, eviction and NULL-robustness without hardware.
 */

#include <stdio.h>
#include "../sms_rx.h"

/* ── capture sink ─────────────────────────────────────────────────── */
#define CAP_MAX 32
static struct { char sender[64]; char text[4096]; char ts[32]; } cap[CAP_MAX];
static int cap_n;

static void tcopy(char *d, const char *s, int max)
{
	int i = 0;
	for (; s && s[i] && i < max - 1; i++) d[i] = s[i];
	d[i] = '\0';
}
static int streq(const char *a, const char *b)
{
	int i = 0;
	for (; a[i] && b[i]; i++) if (a[i] != b[i]) return 0;
	return a[i] == b[i];
}
static void cap_emit(const char *sender, const char *text, const char *ts)
{
	if (cap_n >= CAP_MAX) return;
	tcopy(cap[cap_n].sender, sender, 64);
	tcopy(cap[cap_n].text,   text,   4096);
	tcopy(cap[cap_n].ts,     ts,     32);
	cap_n++;
}

/* Fill `buf` with `n` copies of `c` (n < 4096), return it. */
static char g_tmp[8][4096];
static const char *rep(int slot, char c, int n)
{
	int i;
	for (i = 0; i < n && i < 4095; i++) g_tmp[slot][i] = c;
	g_tmp[slot][i] = '\0';
	return g_tmp[slot];
}

/* ── test driver ──────────────────────────────────────────────────── */
static int g_fail;
static void reset(void) { cap_n = 0; sms_rx_init(cap_emit); }

static void check(const char *name, int cond)
{
	printf("  %-42s %s\n", name, cond ? "PASS" : "*** FAIL ***");
	if (!cond) g_fail = 1;
}

#define UCS2 1
#define GSM  0

int main(void)
{
	printf("sms_rx unit test\n");

	/* S1: single short standalone → emitted immediately. */
	reset();
	sms_rx_part("VNM", "0101", "Hi there", 8, UCS2, 0);
	check("S1 single short: 1 msg", cap_n == 1 && streq(cap[0].text, "Hi there"));

	/* S2: two SEPARATE short messages, same sender, close in time → stay
	 * two messages (the old bug merged them / stuck words together). */
	reset();
	sms_rx_part("A", "", "First msg", 9, UCS2, 0);
	sms_rx_part("A", "", "Second msg", 10, UCS2, 100);
	check("S2 two separate short: 2 msgs",
	      cap_n == 2 && streq(cap[0].text, "First msg") &&
	      streq(cap[1].text, "Second msg"));

	/* S3: real 2-part message (full 67 + short tail) → one joined message. */
	reset();
	{
		char exp[256]; int i = 0;
		const char *p1 = rep(0, 'x', 67);
		for (; p1[i]; i++) exp[i] = p1[i];
		const char *tl = "end"; int j = 0;
		for (; tl[j]; j++) exp[i + j] = tl[j];
		exp[i + j] = '\0';
		sms_rx_part("A", "", p1, 67, UCS2, 0);
		sms_rx_part("A", "", "end", 3, UCS2, 2000);
		check("S3 two-part join: 1 msg", cap_n == 1 && streq(cap[0].text, exp));
	}

	/* S4: 3-part message full+full+short → one joined message. */
	reset();
	sms_rx_part("A", "", rep(0, 'a', 67), 67, UCS2, 0);
	sms_rx_part("A", "", rep(1, 'b', 67), 67, UCS2, 2000);
	sms_rx_part("A", "", "cc", 2, UCS2, 4000);
	check("S4 three-part join: 1 msg", cap_n == 1);
	{
		int ok = cap_n == 1;
		const char *t = cap[0].text; int i;
		for (i = 0; ok && i < 67; i++) ok = t[i] == 'a';
		for (i = 67; ok && i < 134; i++) ok = t[i] == 'b';
		ok = ok && t[134] == 'c' && t[135] == 'c' && t[136] == '\0';
		check("S4 joined content correct", ok);
	}

	/* S5: gap between FULL parts exceeds the safety window → the buffered
	 * full part is flushed on tick, tail arrives as its own message. This is
	 * the accepted residual of text mode (no worse than a lone long SMS). */
	reset();
	sms_rx_part("A", "", rep(0, 'a', 67), 67, UCS2, 0);
	sms_rx_tick(6000);
	sms_rx_part("A", "", "tail", 4, UCS2, 7000);
	check("S5 over-window split: 2 msgs", cap_n == 2);

	/* S6: final short part delayed but within the window → still joins. */
	reset();
	sms_rx_part("A", "", rep(0, 'a', 67), 67, UCS2, 0);
	sms_rx_part("A", "", "zz", 2, UCS2, 4000);
	check("S6 in-window final join: 1 msg", cap_n == 1);

	/* S7: GSM 7-bit multipart (153 septets full + short). */
	reset();
	sms_rx_part("INFO", "", rep(0, 'g', 153), 153, GSM, 0);
	sms_rx_part("INFO", "", "!", 1, GSM, 1000);
	check("S7 gsm multipart join: 1 msg", cap_n == 1);

	/* S8: standalone exactly 67 UCS2 chars → looks 'full', flushed by timer. */
	reset();
	sms_rx_part("A", "", rep(0, 'k', 67), 67, UCS2, 0);
	check("S8 exact-67 single: waits", cap_n == 0);
	sms_rx_tick(6000);
	check("S8 exact-67 single: flushed", cap_n == 1);

	/* S9: interleaved multipart from two senders stay separate + correct. */
	reset();
	sms_rx_part("A", "", rep(0, 'a', 67), 67, UCS2, 0);
	sms_rx_part("B", "", rep(1, 'b', 67), 67, UCS2, 10);
	sms_rx_part("A", "", "A2", 2, UCS2, 100);   /* flush A */
	sms_rx_part("B", "", "B2", 2, UCS2, 110);   /* flush B */
	check("S9 interleaved senders: 2 msgs",
	      cap_n == 2 && streq(cap[0].sender, "A") &&
	      streq(cap[1].sender, "B"));

	/* S10: NULL / empty robustness — must not crash, must not emit garbage. */
	reset();
	sms_rx_part(0, 0, 0, 0, UCS2, 0);
	sms_rx_part("A", 0, "", 0, UCS2, 0);
	sms_rx_tick(999999);
	check("S10 null/empty: no crash, no emit", cap_n == 0);

	/* S11: eviction — 5 senders each hold a full part (only 4 slots); the
	 * 5th evicts the oldest, emitting what it held rather than dropping it. */
	reset();
	sms_rx_part("S1", "", rep(0, '1', 67), 67, UCS2, 0);
	sms_rx_part("S2", "", rep(1, '2', 67), 67, UCS2, 1);
	sms_rx_part("S3", "", rep(2, '3', 67), 67, UCS2, 2);
	sms_rx_part("S4", "", rep(3, '4', 67), 67, UCS2, 3);
	check("S11 four buffered: none emitted yet", cap_n == 0);
	sms_rx_part("S5", "", rep(4, '5', 67), 67, UCS2, 4);
	check("S11 fifth evicts oldest (1 emit)",
	      cap_n == 1 && streq(cap[0].sender, "S1"));

	printf(g_fail ? "\nRESULT: FAIL\n" : "\nRESULT: OK\n");
	return g_fail ? 1 : 0;
}
