/* ============================================================
 * intc.h
 * ------------------------------------------------------------
 * AM335x INTC interrupt-controller driver interface.
 * ============================================================ */

#ifndef INTC_H
#define INTC_H

#include "types.h"

/* ============================================================
 * INTC Hardware Definitions
 * ============================================================ */

/* INTC Base Address */
#define INTC_BASE           0x48200000

/* INTC Control Registers */
#define INTC_REVISION       0x00
#define INTC_SYSCONFIG      0x10
#define INTC_SYSSTATUS      0x14
#define INTC_SIR_IRQ        0x40
#define INTC_SIR_FIQ        0x44
#define INTC_CONTROL        0x48
#define INTC_PROTECTION     0x4C
#define INTC_IDLE           0x50
#define INTC_IRQ_PRIORITY   0x60
#define INTC_FIQ_PRIORITY   0x64
#define INTC_THRESHOLD      0x68

/* INTC Bank Registers (4 banks, 32 IRQs each) */
#define INTC_ITR(n)         (0x80 + ((n) * 0x20))
#define INTC_MIR(n)         (0x84 + ((n) * 0x20))
#define INTC_MIR_CLEAR(n)   (0x88 + ((n) * 0x20))
#define INTC_MIR_SET(n)     (0x8C + ((n) * 0x20))
#define INTC_ISR_SET(n)     (0x90 + ((n) * 0x20))
#define INTC_ISR_CLEAR(n)   (0x94 + ((n) * 0x20))
#define INTC_PENDING_IRQ(n) (0x98 + ((n) * 0x20))
#define INTC_PENDING_FIQ(n) (0x9C + ((n) * 0x20))

/* INTC ILR registers (priority + FIQ/IRQ routing) */
#define INTC_ILR(m)         (0x100 + ((m) * 4))

/* INTC_SIR_IRQ bit fields */
#define ACTIVEIRQ_MASK      0x7F        /* Bits 6:0 */
#define SPURIOUSIRQ_MASK    0xFFFFFF80  /* Bits 31:7 */

/* INTC_CONTROL bits */
#define NEWIRQAGR           (1 << 0)
#define NEWFIQAGR           (1 << 1)

/* ============================================================
 * INTC Driver API
 * ============================================================ */

void intc_init(void);

/* Returns 128 for a spurious IRQ. */
uint32_t intc_get_active_irq(void);

/* CRITICAL: must be called for EVERY IRQ — valid, spurious, or
 * unhandled — otherwise INTC keeps the line asserted. */
void intc_eoi(void);

/* Precondition: handler must be registered before enabling. */
void intc_enable_irq(uint32_t irq_num);

void intc_disable_irq(uint32_t irq_num);

/* Lower priority value = higher priority (0–63). Currently unused —
 * all IRQs run at priority 0. */
void intc_set_priority(uint32_t irq_num, uint32_t priority);

#endif /* INTC_H */