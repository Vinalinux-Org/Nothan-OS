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

#define IER_RHR_IT		(1 << 0)

#define IIR_IT_PENDING		(1 << 0)

#define LCR_DLAB		(1 << 7)
#define LCR_8N1			(3 << 0)

#define FCR_FIFO_EN		(1 << 0)
#define FCR_RX_TRIG_8		(2 << 6)

#define LSR_DR			(1 << 0)
#define LSR_THRE		(1 << 5)

#define UART_IRQ		72

void uart_init(void);
void uart_putchar(int c);
int uart_getchar(void);

#endif /* _UART_H */
