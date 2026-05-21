/*
 * drivers/tty/serial/serial_core.c — generic serial RX framework
 *
 * Compatibility wrapper around the shared console input queue.
 * UART hardware drivers push received bytes via uart_serial_rx_push();
 * userspace (sys_read) consumes via uart_getc() / uart_rx_available().
 */

#include "uart.h"
#include "wait_queue.h"
#include "nothan/printk.h"
#include "nothan/tty.h"

static volatile uint32_t     uart_irq_fire_count;

/* Exposed extern for callers that want to wait_event on it. */
wait_queue_head_t uart_rx_wq = { .head = 0 };

int uart_serial_rx_push(uint8_t ch)
{
    int ret = tty_receive_char(ch);
    wake_up(&uart_rx_wq);
    return ret;
}

void uart_serial_rx_irq_count(void)
{
    uart_irq_fire_count++;
}

int uart_getc(void)
{
    return tty_get_char();
}

int uart_rx_available(void)
{
    return tty_input_available();
}

void uart_rx_clear(void)
{
    tty_input_clear();
}

uint32_t uart_get_irq_fire_count(void)
{
    return uart_irq_fire_count;
}
