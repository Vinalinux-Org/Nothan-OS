/*
 * include/vinix/serial_core.h — serial port framework
 *
 * Drivers implement uart_ops and call uart_add_one_port().
 * The core handles ring buffers, RX wait queue, and tty layer wiring.
 */

#ifndef VINIX_SERIAL_CORE_H
#define VINIX_SERIAL_CORE_H

#include "types.h"
#include "vinix/tty.h"

struct uart_port;

struct uart_ops {
    uint32_t (*tx_empty)    (struct uart_port *port);
    void     (*start_tx)    (struct uart_port *port);
    void     (*stop_tx)     (struct uart_port *port);
    void     (*stop_rx)     (struct uart_port *port);
    int      (*startup)     (struct uart_port *port);
    void     (*shutdown)    (struct uart_port *port);
    void     (*set_termios) (struct uart_port *port, uint32_t cflag);
    const char *(*type)     (struct uart_port *port);
};

struct uart_port {
    void                  __attribute__((aligned(4))) *membase;  /* iomem */
    uint32_t                irq;
    uint32_t                uartclk;
    uint32_t                fifosize;
    int                     line;
    const struct uart_ops  *ops;
    void                   *priv;
};

struct uart_driver {
    const char         *driver_name;
    const char         *dev_name;     /* /dev/<dev_name>NN */
    int                 major;
    int                 minor;
    int                 nr;
    struct tty_driver  *tty_drv;
    struct uart_port  **ports;
};

int  uart_register_driver(struct uart_driver *drv);
int  uart_add_one_port    (struct uart_driver *drv, struct uart_port *port);
void uart_remove_one_port (struct uart_driver *drv, struct uart_port *port);

#endif /* VINIX_SERIAL_CORE_H */
