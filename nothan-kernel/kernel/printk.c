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
#include <nothan/wait.h>
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
 * pointer work, and a kernel thread does the waiting with interrupts ENABLED.
 * The CPU still spends the same time feeding the UART; it just no longer spends
 * it deaf.
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
static unsigned int  log_tail;		/* consumer: klogd or a flush */
static unsigned long log_dropped;	/* characters overwritten when full */

/*
 * Synchronous until the log thread is actually running, and again from the
 * moment panic() starts. Both are cases where there may be no later.
 */
static bool log_sync = true;

/*
 * Initialised statically, not in klog_init().
 *
 * wake_up() reads task_list before it does anything else, and an all-zero
 * list head is not an empty list - list_empty() compares next against the head
 * address, so a NULL next reads as NON-empty and the waker walks into it. The
 * ordering that keeps that from happening (nothing wakes until klogd has run)
 * is real but invisible, and printk is called from everywhere including places
 * that will exist later. A self-referential initialiser makes the queue valid
 * from the first instruction of the kernel and removes the question.
 */
static struct wait_queue_head log_wq = {
	.task_list = { &log_wq.task_list, &log_wq.task_list },
};

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
void printk_flush(void)
{
	for (;;) {
		unsigned long flags;
		char c;

		local_irq_save(flags);
		if (log_tail == log_head) {
			local_irq_restore(flags);
			return;
		}
		c = log_buf[log_tail & LOG_BUF_MASK];
		log_tail++;
		local_irq_restore(flags);

		if (c == '\n')
			uart_putchar('\r');
		uart_putchar(c);
	}
}

/**
 * printk_panic_mode() - stop deferring; print everything from here on inline
 *
 * panic() calls this before its first printk. Waking a thread to do the
 * printing assumes the scheduler will run again, which is not a safe
 * assumption at the point where the kernel has decided to stop - and the panic
 * message is the one message that must never be the one that got left in RAM.
 */
void printk_panic_mode(void)
{
	log_sync = true;
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

	if (log_sync)
		printk_flush();
	else
		wake_up(&log_wq);

	return ret;
}

/*
 * The log thread. Sleeps until there is something to print, then prints it
 * with interrupts enabled - which is the entire reason it exists.
 *
 * It reports dropped characters straight to the UART rather than through
 * printk(), because printk() would append to the ring it is currently draining
 * and the notice about losing data could itself be lost.
 */
static void klog_thread(void *arg)
{
	unsigned long reported = 0;

	/*
	 * Deferral starts HERE, not when the thread was created. Between
	 * creation and first run nothing drains, so switching earlier would
	 * silently bank the rest of boot into a 16 KB ring and overwrite the
	 * beginning of it. The first thing this thread does is prove it is
	 * running by draining what boot left behind.
	 */
	printk_flush();
	log_sync = false;

	for (;;) {
		wait_event(&log_wq, log_head != log_tail);
		printk_flush();

		if (log_dropped != reported) {
			const char *m = "[LOG] ring overflowed, characters lost\r\n";

			reported = log_dropped;
			while (*m)
				uart_putchar(*m++);
		}
	}
}

/**
 * klog_init() - start the log thread
 *
 * Must run after init exists (so the thread has a parent) and before the first
 * schedule(). Until the thread first runs, printk stays synchronous, so a
 * failure here costs latency and nothing else.
 */
void klog_init(void)
{
	struct task_struct *t;

	init_waitqueue_head(&log_wq);

	t = task_create(klog_thread, NULL, DEFAULT_PRIO, "klogd");
	if (!t) {
		printk("[LOG] cannot start klogd - console stays synchronous\n");
		return;
	}
	enqueue_task(&runqueue, t);
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
