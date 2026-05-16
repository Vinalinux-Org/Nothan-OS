/*
 * kernel/irqchip/irqchip.c — IRQ chip registration
 */

#include "nothan/irqchip.h"
#include "nothan/printk.h"
#include "nothan/errno.h"

static struct irq_chip *root_chip;

int irqchip_register(struct irq_chip *chip)
{
    if (!chip) return -EINVAL;
    root_chip = chip;
    pr_info("[IRQCHIP] %s registered as root chip\n",
            chip->name ? chip->name : "?");
    return 0;
}

struct irq_chip *irqchip_get_root(void)
{
    return root_chip;
}
