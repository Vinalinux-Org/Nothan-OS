/*
 * include/uart.h — AM335x UART0 driver interface
 */

#ifndef UART_H
#define UART_H

#include "types.h"
#include <stdarg.h>



/* Ring buffer size (must be power of 2 for efficient modulo) */
#define UART_RX_BUFFER_SIZE     256



/* Blocks until TX FIFO ready. */
void uart_putc(char c);

void uart_puts(const char *s);

/* uart_printf is the historical name; new code uses printk
 * (or pr_info / pr_err / ... from vinix/printk.h). Both expand
 * to the same kernel-wide formatter. */
#include "vinix/printk.h"
#define uart_printf  printk



/* Non-blocking — returns -1 if the ring buffer is empty.
 * NOT safe to call from IRQ context. */
int uart_getc(void);

/* Peek without consuming. */
int uart_rx_available(void);

/* NOT safe in IRQ context. */
void uart_rx_clear(void);

uint32_t uart_get_irq_fire_count(void);

/* sys_read waits on this; serial_core's rx_push wakes it per byte. */
#include "wait_queue.h"
extern wait_queue_head_t uart_rx_wq;

/* Driver IRQ -> serial_core glue. */
int  uart_serial_rx_push(uint8_t ch);
void uart_serial_rx_irq_count(void);

#endif /* UART_H */