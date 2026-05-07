/*
 * include/vinix/mdio.h — MDIO bus interface for Ethernet PHY management
 */

#ifndef VINIX_MDIO_H
#define VINIX_MDIO_H

#include "types.h"

struct mdio_bus {
    const char  *name;
    uint32_t     base;
    void        *priv;
};

int              mdio_add_bus(struct mdio_bus *bus);
struct mdio_bus *mdio_get_bus(int nr);
int              mdio_read  (struct mdio_bus *bus, uint8_t phy_addr, uint8_t reg);
int              mdio_write (struct mdio_bus *bus, uint8_t phy_addr, uint8_t reg, uint16_t val);
void             mdio_enable(struct mdio_bus *bus);

#endif /* VINIX_MDIO_H */
