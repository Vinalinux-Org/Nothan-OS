#ifndef AM335X_UART_H
#define AM335X_UART_H

#define UART0_BASE      0x44E09000

#define UART0_THR       (UART0_BASE + 0x00)
#define UART0_RHR       (UART0_BASE + 0x00)
#define UART0_DLL       (UART0_BASE + 0x00)
#define UART0_DLH       (UART0_BASE + 0x04)
#define UART0_IER       (UART0_BASE + 0x04)
#define UART0_FCR       (UART0_BASE + 0x08)
#define UART0_LCR       (UART0_BASE + 0x0C)
#define UART0_MCR       (UART0_BASE + 0x10)
#define UART0_LSR       (UART0_BASE + 0x14)
#define UART0_MDR1      (UART0_BASE + 0x20)
#define UART0_SYSC      (UART0_BASE + 0x54)
#define UART0_SYSS      (UART0_BASE + 0x58)

#define UART_LCR_DIV_EN     (1 << 7)
#define UART_LCR_8N1        0x03

#define UART_LSR_RXFIFOE    (1 << 0)
#define UART_LSR_TXFIFOE    (1 << 5)
#define UART_LSR_TEMT       (1 << 6)

#define UART_16X_MODE       0x00
#define UART_DISABLE        0x07

#endif /* AM335X_UART_H */
