/*
 * kernel/printk.c - Formatted kernel console output (vsnprintf + printk)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <nothan/printk.h>
#include <nothan/uart.h>
#include <nothan/sched.h>
#include <asm/irqflags.h>

/*
 * Minimal vsnprintf — %s %d %i %u %x %X %p %% + width + zero-pad
 */

static const char hex_lower[] = "0123456789abcdef";

static int number(char *buf, unsigned long size, unsigned long val,
		  int base, int width, int sign, int upper, int zp)
{
	char tmp[40];
	const char *digits = upper ? "0123456789ABCDEF" : hex_lower;
	int neg = 0, wr = 0, len = 0;
	unsigned long v;

	if (sign && (long)val < 0) {
		neg = 1;
		val = -(long)val;
	}

	v = val;
	do {
		tmp[len++] = digits[v % base];
		v /= base;
	} while (v);

	int nd = len + neg;
	int pad = (width > nd) ? width - nd : 0;

	if (!zp)
		while (pad-- > 0 && wr + 1 < (int)size)
			buf[wr++] = ' ';

	if (neg && wr + 1 < (int)size)
		buf[wr++] = '-';

	if (zp)
		while (pad-- > 0 && wr + 1 < (int)size)
			buf[wr++] = '0';

	while (len > 0 && wr + 1 < (int)size)
		buf[wr++] = tmp[--len];

	return wr;
}

/**
 * vsnprintf() - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer
 * @fmt: The format string
 * @args: Arguments for the format string
 *
 * Return: The number of characters that would have been written
 */
int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args)
{
	int pos = 0;

	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			if (pos + 1 < (int)size)
				buf[pos] = *fmt;
			pos++;
			continue;
		}

		if (*++fmt == '%') {
			if (pos + 1 < (int)size)
				buf[pos] = '%';
			pos++;
			continue;
		}

		int width = 0, zero_pad = 0;

		if (*fmt == '0') { zero_pad = 1; fmt++; }
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');

		int is_long = (*fmt == 'l');
		if (is_long) fmt++;
		if (*fmt == 'l') fmt++;

		switch (*fmt) {
		case 's': {
			const char *s = va_arg(args, const char *);
			if (!s) s = "(null)";
			int slen = 0;
			while (s[slen]) slen++;
			int pad = (width > slen) ? width - slen : 0;
			while (pad-- > 0 && pos + 1 < (int)size)
				buf[pos++] = ' ';
			for (int i = 0; s[i]; i++) {
				if (pos + 1 < (int)size)
					buf[pos] = s[i];
				pos++;
			}
			break;
		}

		case 'd':
		case 'i':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, long)
					      : va_arg(args, int),
				      10, width, 1, 0, zero_pad);
			break;

		case 'u':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      10, width, 0, 0, zero_pad);
			break;

		case 'x':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      16, width, 0, 0, zero_pad);
			break;

		case 'X':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      16, width, 0, 1, zero_pad);
			break;

		case 'p':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      (unsigned long)va_arg(args, void *),
				      16, 8, 0, 0, 1);
			break;

		default:
			if (pos + 1 < (int)size)
				buf[pos] = *fmt;
			pos++;
			break;
		}
	}

	if ((int)size > 0)
		buf[pos < (int)size ? pos : (int)size - 1] = '\0';

	return pos;
}

/**
 * printk() - Print a formatted string to the kernel console
 * @fmt: The format string
 * @...: Arguments for the format string
 *
 * This function formats a string and outputs it to the UART console.
 * It translates newline characters to carriage return + newline.
 *
 * Return: The number of characters written
 */
/* ===================================================================
 * The log ring, and why printk no longer writes to the UART itself
 *
 * printk() used to mask IRQs and busy-wait on the UART transmit register, one
 * character at a time, for the whole line. At 115200 8N1 a character is 86.8 us
 * of waiting, so an ordinary 80-character line held interrupts off for about
 * SEVEN MILLISECONDS.
 *
 * Nothing in the kernel as it stood minded very much. Everything about to be
 * added does:
 *
 *   - audio at 48 kHz with a 512-frame period has 10.7 ms between refills, so
 *     one log line eats two thirds of the margin and the buffer underruns;
 *   - a 1500-byte frame arrives on 100 Mbit Ethernet every 120 us, so 7 ms
 *     with interrupts off is about 58 frames the controller had nowhere to put.
 *
 * It was also self-inflicted in the worst place: schedule() calls reap_dead(),
 * which pr_debug()s, and the IRQ preemption path calls schedule() - so a task
 * exiting at the wrong moment put a multi-millisecond UART wait INSIDE an
 * interrupt.
 *
 * So printk() now only formats and appends to a ring in RAM, which is bounded
 * pointer work, and the waiting happens later, with interrupts ENABLED. The CPU
 * still spends the same time feeding the UART; it just no longer spends it deaf.
 *
 * WHO DOES THE WAITING, given there is no thread to do it. A kernel thread was
 * the obvious answer and is not an available one: kernel threads share one
 * address space, which is the shared mutable state
 * Documentation/design-philosophy.md §5.1.1 removes by construction. So the
 * draining is done by contexts that already exist and already have the CPU:
 *
 *   the idle loop      drains without limit - it has nothing better to do, and
 *                      "nothing runnable" is exactly when the UART is free
 *   syscall return     drains a bounded handful, so a busy system that never
 *                      reaches idle still moves the log along without any one
 *                      syscall stalling for a whole line
 *
 * The honest gap in that: a kernel path that neither idles nor returns to user
 * space can sit on a full ring. Everything that matters in such a path is an
 * error or a fault, and those flush for themselves - see below.
 *
 * WHAT IS GIVEN UP, said plainly: a line sits in RAM for a moment before it
 * reaches the wire, so a machine that stops dead can lose the last few lines -
 * and on a machine whose only instrument is the UART that is not a small thing
 * to trade. It is bought back at the three points where it actually matters:
 * pr_err() flushes, the fault handlers flush, and panic() switches the whole
 * mechanism back to synchronous before printing a word. Routine chatter is
 * deferred; anything that might be somebody's last words is not.
 * =================================================================== */

#define LOG_BUF_SHIFT	14
#define LOG_BUF_SIZE	(1u << LOG_BUF_SHIFT)
#define LOG_BUF_MASK	(LOG_BUF_SIZE - 1u)

static char log_buf[LOG_BUF_SIZE];

/*
 * Free-running counters, masked only when indexing. Their DIFFERENCE is the
 * occupancy and stays correct across unsigned wraparound, which a pair of
 * wrapped indices would not - those cannot tell full from empty without a
 * spare slot or a flag.
 */
static unsigned int  log_head;		/* producer: printk */
static unsigned int  log_tail;		/* consumer: idle, syscall return, or a flush */
static unsigned long log_dropped;	/* characters overwritten when full */

/*
 * Set once panic() starts, and never cleared: from that point printing is
 * synchronous again, because the drain points are the idle loop and syscall
 * return, and panic()'s whole contract is that neither will ever run again.
 */
static bool log_panicking;

/* Append to the ring, dropping the OLDEST when full.
 *
 * Dropping old rather than new because a full ring means the reader is behind,
 * and what a reader that is behind most needs is the newest lines - the ones
 * describing whatever is currently going wrong. Discarding the incoming line
 * instead would go quiet exactly during the storm worth reading.
 */
static void log_write(const char *s)
{
	unsigned long flags;

	local_irq_save(flags);
	for (; *s; s++) {
		if (log_head - log_tail >= LOG_BUF_SIZE) {
			log_tail++;
			log_dropped++;
		}
		log_buf[log_head & LOG_BUF_MASK] = *s;
		log_head++;
	}
	local_irq_restore(flags);
}

/*
 * Drain the ring to the UART in the caller's context.
 *
 * The mask is taken and dropped PER CHARACTER, around the ring surgery only,
 * and never across the transmit wait. That is the whole point of the rewrite:
 * the interrupts-off window goes from the length of a line to a few
 * instructions. Two drainers running at once interleave characters rather than
 * corrupting the ring, which is untidy and survivable; holding the mask across
 * the wait to keep it tidy would put the old bug back.
 */
/*
 * Move one FIFO-sized burst from the ring to the UART.
 *
 * The characters are lifted out under the mask into a local buffer first, and
 * only then handed to the hardware. That ordering is what keeps the
 * interrupts-off window to a memcpy: the transmit — the part that takes real
 * time — happens with the mask already dropped. Draining straight from the ring
 * to the register would put the wait back inside the critical section, which is
 * the whole bug this file exists to fix.
 *
 * Return: characters written; 0 means the ring is empty or (when @wait is 0)
 * the FIFO was busy.
 */
static unsigned int drain_burst(int wait)
{
	char out[UART_TX_FIFO_DEPTH];
	unsigned int n = 0;
	unsigned long flags;

	local_irq_save(flags);
	while (n < UART_TX_FIFO_DEPTH - 1 && log_tail != log_head) {
		char c = log_buf[log_tail & LOG_BUF_MASK];

		/* Expanded here rather than at the UART so the burst length is
		 * known before anything is written; a \r appended down there
		 * could push the count past the FIFO depth. */
		if (c == '\n')
			out[n++] = '\r';
		out[n++] = c;
		log_tail++;
	}
	local_irq_restore(flags);

	if (!n)
		return 0;

	return uart_console_tx(out, n, wait);
}

void printk_flush(void)
{
	while (drain_burst(1))
		;
}

/**
 * printk_drain_some() - help move the log along, without ever blocking
 *
 * The syscall return path uses this. A process that keeps the CPU busy never
 * reaches the idle loop, so without it the log would stall for exactly as long
 * as the machine was working hardest - which is when it is most worth reading.
 *
 * It refuses to wait, and that is the point rather than a limitation. A
 * blocking drain here would charge an ordinary syscall up to a FIFO's worth of
 * serial time, which at 115200 is milliseconds, to do the logging a favour. If
 * the FIFO is busy this does nothing at all and the next syscall tries again.
 */
void printk_drain_some(unsigned int max_bursts)
{
	while (max_bursts-- && drain_burst(0))
		;
}

/**
 * printk_panic_mode() - stop deferring; print everything from here on inline
 *
 * panic() calls this before its first printk. Deferring assumes something will
 * come along later to drain - the idle loop, or a syscall returning - and
 * panic()'s contract is precisely that nothing will. The panic message is the
 * one message that must never be the one left sitting in RAM.
 */
void printk_panic_mode(void)
{
	log_panicking = true;
}

int printk(const char *fmt, ...)
{
	char buf[256];
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	log_write(buf);

	/*
	 * Defer only once there is somebody to defer TO. Before the first
	 * context switch there is no idle loop and no syscall return, so a
	 * queued line would sit in the ring until whatever is being set up
	 * either finishes or hangs — and a probe that hangs is exactly when its
	 * last line has to already be on the wire. sched_running is the
	 * scheduler's own record of that moment, so the two cannot drift apart.
	 */
	if (!sched_running || log_panicking)
		printk_flush();

	return ret;
}


/*
 * Division helpers — Cortex-A8 ARM mode has no HW divide
 */

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d)
{
	unsigned int q = 0, r = 0;

	for (int i = 31; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) {
			r -= d;
			q |= (1U << i);
		}
	}
	return q;
}

unsigned long long __aeabi_uidivmod(unsigned int n, unsigned int d)
{
	unsigned int q = 0, r = 0;

	for (int i = 31; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) {
			r -= d;
			q |= (1U << i);
		}
	}
	return ((unsigned long long)r << 32) | q;
}
