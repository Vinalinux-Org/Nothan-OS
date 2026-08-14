#ifndef _UART_H
#define _UART_H

/*
 * UART0 mapped via L4_WKUP (VA 0xF0E00000 → PA 0x44E00000).
 * UART0 is at PA 0x44E09000, VA 0xF0E09000.
 */
#define UART_BASE		0xF0E09000

#define UART_THR		0x00
#define UART_RHR		0x00
#define UART_DLL		0x00	
#define UART_IER		0x04
#define UART_DLH		0x04	
#define UART_IIR		0x08
#define UART_FCR		0x08
#define UART_LCR		0x0C
#define UART_LSR		0x14

/*
 * TRM Ch19: 64-byte TX and RX FIFOs; UART_TXFIFO_LVL (read-only) is the number
 * of bytes currently queued, so 64 minus it is exactly how many may be written
 * without overflowing.  Knowing the real figure beats inferring one from LSR,
 * which only distinguishes "completely empty" from "not".
 */
#define UART_TXFIFO_LVL		0x68
#define UART_TX_FIFO_SIZE	64u
#define UART_MDR1		0x20	/* mode: 0x07=disabled (reset), 0x00=UART 16x */

#define IER_RHR_IT		(1 << 0)
#define IER_THR_IT		(1 << 1)	/* THR empty — TRM Ch19, IER[1] */

/*
 * IIR (TRM Ch19, UART_IIR_UART, offset 0x08, read-only):
 *   [0]   IT_PENDING  0 = an interrupt is pending, 1 = none.  Note the sense.
 *   [5:1] IT_TYPE     which one, highest priority first
 *
 * Reading IIR is also one of the two ways to clear a pending THR interrupt,
 * the other being a write to THR.  A handler that does neither leaves the line
 * asserted and is re-entered immediately.
 */
#define IIR_IT_PENDING		(1 << 0)
#define IIR_IT_TYPE_SHIFT	1
#define IIR_IT_TYPE_MASK	0x1F

#define IIR_TYPE_MODEM		0x0
#define IIR_TYPE_THR		0x1
#define IIR_TYPE_RHR		0x2
#define IIR_TYPE_LINE_STATUS	0x3
#define IIR_TYPE_RX_TIMEOUT	0x6
#define IIR_TYPE_XOFF		0x8
#define IIR_TYPE_MODEM_STATE	0x10

#define LCR_DLAB		(1 << 7)
#define LCR_8N1			(3 << 0)

#define FCR_FIFO_EN		(1 << 0)
#define FCR_RX_TRIG_8		(2 << 6)

#define LSR_DR			(1 << 0)

/*
 * TRM Ch19, LSR:
 *   [5] TXFIFOE — transmit hold register empty, "transmission not necessarily
 *                 completed".  Room to queue another byte, nothing more.
 *   [6] TXSRE   — both the TX FIFO *and* the shift register are empty, i.e.
 *                 every byte handed over has actually left the pin.
 *
 * Waiting on [5] is right before writing a byte and wrong before touching the
 * UART's configuration: up to a FIFO's worth of log can still be in flight.
 */
#define LSR_THRE		(1 << 5)
#define LSR_TXSRE		(1 << 6)

#define UART_IRQ		72

/*
 * UART1 (modem) — L4_PER, mapped VA 0xF0000000 → PA 0x48000000 (mmu.c).
 * UART1 at PA 0x48022000, VA 0xF0022000. IRQ 73 (Ch.6). Clock control is
 * CM_PER_UART1_CLKCTRL at CM_PER+0x6C (Ch.8). NOTE: the old "CM_PER_UART0"
 * name was a misnomer — 0x6C is UART1's clkctrl; UART0 is clocked by the
 * bootloader via CM_WKUP, so the kernel leaves UART0's clock alone.
 */
#define UART0_PA		0x44E09000
#define UART1_PA		0x48022000
#define UART1_VA		0xF0022000	/* UART1 register VA (L4_PER) */
#define UART1_IRQ		73
#define CM_PER_UART1_CLKCTRL	0xF0E0006C

void uart_init(void);
int uart_getchar(void);

/*
 * Single character, unsynchronised.  One character cannot be interleaved with
 * anything, so it needs no masking — but that also means it is NOT a building
 * block for strings: a loop over it is exactly the unprotected path that used
 * to cut userspace lines in half.  Use console_write()/console_puts() for
 * anything longer than one byte.  Kept for paths that must not depend on the
 * console being in a sane state, such as a future panic dump.
 */
void uart_putchar(int c);

/*
 * Atomic console output.  Every multi-byte write to the console must go
 * through one of these: they mask around the whole run, so one message cannot
 * be interleaved with another mid-string.  uart_putchar() is a single
 * character and needs no such protection.
 *
 *   console_write() — raw bytes, for callers holding a complete message
 *   console_puts()  — NUL-terminated, expands bare newlines to CR LF
 */
int  console_read(char *buf, size_t count);	/* blocks until data */
int  console_write(const char *buf, size_t count);
void console_puts(const char *s);

/* Empty the console ring synchronously.  For panic() only — see the comment
 * on the definition. */
void console_flush_panic(void);

/* Which source each UART interrupt came from.  Empty unless CONFIG_IRQ_TIMING. */
void console_dump_irq_sources(void);

#endif /* _UART_H */
