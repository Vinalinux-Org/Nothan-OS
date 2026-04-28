/*
 * include/vinix/irqchip.h — interrupt controller vtable
 *
 * struct irq_chip defines the interface for IRQ chip drivers.
 * Currently the AM335x INTC is driven directly; this header defines
 * the contract for future multi-controller configurations.
 */

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
