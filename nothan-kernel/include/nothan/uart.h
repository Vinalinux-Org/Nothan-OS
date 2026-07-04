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
#define UART_MDR1		0x20	/* mode: 0x07=disabled (reset), 0x00=UART 16x */

#define IER_RHR_IT		(1 << 0)

#define IIR_IT_PENDING		(1 << 0)

#define LCR_DLAB		(1 << 7)
#define LCR_8N1			(3 << 0)

#define FCR_FIFO_EN		(1 << 0)
#define FCR_RX_TRIG_8		(2 << 6)

#define LSR_DR			(1 << 0)
#define LSR_THRE		(1 << 5)

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
void uart_putchar(int c);
int uart_getchar(void);

#endif /* _UART_H */
