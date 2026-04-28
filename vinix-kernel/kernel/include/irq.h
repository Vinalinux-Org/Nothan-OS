/* ============================================================
 * irq.h
 * ------------------------------------------------------------
 * IRQ framework interface.
 * ============================================================ */

#ifndef IRQ_H
#define IRQ_H

#include "types.h"

/* ============================================================
 * IRQ Configuration
 * ============================================================ */

/* Maximum number of IRQ sources on AM335x */
#define MAX_IRQS    128

/* Common IRQ numbers (AM335x specific) */
#define IRQ_TIMER0      66
#define IRQ_TIMER1      67
#define IRQ_TIMER2      68
#define IRQ_TIMER3      69
#define IRQ_TIMER4      92
#define IRQ_TIMER5      93
#define IRQ_TIMER6      94
#define IRQ_TIMER7      95

#define IRQ_UART0       72
#define IRQ_UART1       73
#define IRQ_UART2       74
#define IRQ_UART3       44
#define IRQ_UART4       45
#define IRQ_UART5       46

#define IRQ_GPIO0A      96
#define IRQ_GPIO0B      97
#define IRQ_GPIO1A      98
#define IRQ_GPIO1B      99
#define IRQ_GPIO2A      32
#define IRQ_GPIO2B      33
#define IRQ_GPIO3A      62
#define IRQ_GPIO3B      63

/* ============================================================
 * IRQ Handler Type
 * ============================================================ */

typedef void (*irq_handler_t)(void *data);

/* ============================================================
 * IRQ Framework API — Linux-aligned
 * ============================================================ */

void irq_init(void);

/* Register a handler for irq. flags currently unused (reserved for
 * IRQF_SHARED / IRQF_TRIGGER_*); name shows up in /proc/interrupts;
 * dev is opaque cookie passed to handler. Returns 0 / -errno. */
int request_irq(unsigned int irq, irq_handler_t handler,
                unsigned long flags, const char *name, void *dev);

/* Releases a handler previously registered with request_irq. dev must
 * match the cookie used at registration. */
void free_irq(unsigned int irq, void *dev);

/* Unmask / mask an IRQ at the interrupt controller. */
void enable_irq(unsigned int irq);
void disable_irq(unsigned int irq);

/* CRITICAL: always sends EOI — even for spurious/unhandled IRQs. */
void irq_dispatch(void *ctx);

uint32_t irq_get_count(uint32_t irq);

#endif /* IRQ_H */