/* ============================================================
 * irqchip.c
 * ------------------------------------------------------------
 * Generic IRQ chip registry. The kernel-side enable_irq /
 * disable_irq dispatch through whichever struct irq_chip the
 * platform's INTC driver registered here.
 *
 * Single-chip MVP: one global slot. When a second controller
 * (e.g., GPIO IRQ chip on top of INTC) lands, replace this
 * with a per-irq descriptor table mapping irq -> chip.
 * ============================================================ */

#include "vinix/irqchip.h"
#include "vinix/printk.h"
#include "vinix/errno.h"

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
