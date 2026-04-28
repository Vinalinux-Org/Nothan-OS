/* ============================================================
 * vinix/serial_core.h
 * ------------------------------------------------------------
 * Serial port framework — uart_driver wraps tty_driver, drivers
 * implement uart_ops.
 *
 * Driver writers add a struct uart_port (mmio base, irq, fifo
 * size) and an uart_ops vtable, then call uart_add_one_port to
 * plug into serial_core. The core handles ring buffers, RX wait
 * queue, and tty layer wiring.
 *
 * Migration of omap_serial.c on top of this header is part of
 * Phase 2.5 (TTY/serial subsystem core build-out).
 * ============================================================ */

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
