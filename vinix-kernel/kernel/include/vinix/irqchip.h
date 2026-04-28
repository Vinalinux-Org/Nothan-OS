/* ============================================================
 * vinix/irqchip.h
 * ------------------------------------------------------------
 * struct irq_chip — interrupt controller vtable. Each IRQ chip
 * driver (intc, gic, ...) fills this and registers the chip with
 * the generic irq_core dispatcher.
 *
 * VinixOS today drives the single AM3358 INTC directly via
 * intc_enable_irq / intc_disable_irq; this header defines the
 * subsystem contract for the future when multiple controllers
 * (e.g., per-SoC INTC + GPIO IRQ chip) need to coexist.
 * ============================================================ */

#ifndef VINIX_IRQCHIP_H
#define VINIX_IRQCHIP_H

#include "types.h"

struct irq_data {
    uint32_t           irq;
    struct irq_chip   *chip;
    void              *chip_data;
};

struct irq_chip {
    const char *name;
    void (*irq_mask)   (struct irq_data *d);
    void (*irq_unmask) (struct irq_data *d);
    void (*irq_ack)    (struct irq_data *d);
    void (*irq_eoi)    (struct irq_data *d);
    int  (*irq_set_type)(struct irq_data *d, uint32_t type);
};

int               irqchip_register(struct irq_chip *chip);
struct irq_chip  *irqchip_get_root(void);

#endif /* VINIX_IRQCHIP_H */
