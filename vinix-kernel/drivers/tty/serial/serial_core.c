/* ============================================================
 * serial_core.c
 * ------------------------------------------------------------
 * Generic serial framework — owns the RX ring buffer and the
 * wait queue that sys_read blocks on. UART hardware drivers
 * push received bytes via uart_serial_rx_push(); userspace path
 * (devfs /dev/tty, sys_read) consumes via uart_getc /
 * uart_rx_available.
 *
 * Single-port for now (one circular buffer, one wait queue).
 * Multi-port support comes when struct uart_port from
 * vinix/serial_core.h gets a real registry.
 * ============================================================ */

#include "uart.h"
#include "irq.h"
#include "cpu.h"          /* irq_save / irq_restore */
#include "wait_queue.h"
#include "vinix/printk.h"

/* Sized per uart.h to keep ABI for any out-of-tree consumer. */
struct uart_rx_buffer {
    uint8_t           data[UART_RX_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t overflow;
};

static struct uart_rx_buffer rx_buffer;
static volatile uint32_t     uart_irq_fire_count;

/* Exposed extern for callers that want to wait_event on it. */
wait_queue_head_t uart_rx_wq = { .head = 0 };

/* IRQ-context entry — driver pushes one received byte. Wakes
 * exactly one sys_read waiter so the shell makes progress per
 * char. Returns 0 on success, -1 on overflow (byte dropped). */
int uart_serial_rx_push(uint8_t ch)
{
    uint32_t next_head = (rx_buffer.head + 1) % UART_RX_BUFFER_SIZE;

    if (next_head == rx_buffer.tail) {
        rx_buffer.overflow++;
        return -1;
    }

    rx_buffer.data[rx_buffer.head] = ch;
    rx_buffer.head = next_head;

    wake_up(&uart_rx_wq);
    return 0;
}

void uart_serial_rx_irq_count(void)
{
    uart_irq_fire_count++;
}

int uart_getc(void)
{
    uint32_t flags = irq_save();

    if (rx_buffer.head == rx_buffer.tail) {
        irq_restore(flags);
        return -1;
    }

    uint8_t ch = rx_buffer.data[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % UART_RX_BUFFER_SIZE;

    irq_restore(flags);
    return (int)ch;
}

int uart_rx_available(void)
{
    uint32_t head = rx_buffer.head;
    uint32_t tail = rx_buffer.tail;

    if (head >= tail) return head - tail;
    return UART_RX_BUFFER_SIZE - tail + head;
}

void uart_rx_clear(void)
{
    uint32_t flags = irq_save();
    rx_buffer.head = 0;
    rx_buffer.tail = 0;
    irq_restore(flags);
}

uint32_t uart_get_irq_fire_count(void)
{
    return uart_irq_fire_count;
}
