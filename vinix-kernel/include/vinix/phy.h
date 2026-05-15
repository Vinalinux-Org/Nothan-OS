/*
 * include/vinix/phy.h — PHY device state, MII clause 22 register defines
 */
#ifndef VINIX_PHY_H
#define VINIX_PHY_H

#include "types.h"

/* Standard MII registers (IEEE 802.3 clause 22) */
#define MII_BMCR        0
#define MII_BMSR        1
#define MII_PHYSID1     2
#define MII_PHYSID2     3
#define MII_ADVERTISE   4
#define MII_LPA         5

/* BMCR bits */
#define BMCR_RESET      (1 << 15)
#define BMCR_ANENABLE   (1 << 12)
#define BMCR_ANRESTART  (1 << 9)

/* BMSR bits */
#define BMSR_ANEGCOMPLETE  (1 << 5)
#define BMSR_LSTATUS       (1 << 2)

/* LAN8710A identifiers */
#define LAN8710A_PHYID1       0x0007
#define LAN8710A_PHYID2       0xC0F0   /* revision 0 — bits[3:0] vary by silicon */
#define LAN8710A_PHYID2_MASK  0xFFF0

struct mdio_bus;

/**
 * struct phy_device - PHY state
 * @bus:    MDIO bus this PHY lives on
 * @addr:   PHY address (BBB LAN8710A: 0)
 * @link:   1 = link up, 0 = link down
 * @speed:  10 or 100 Mbps
 * @duplex: 0 = half, 1 = full
 */
struct phy_device {
    struct mdio_bus *bus;
    uint8_t          addr;
    int              link;
    int              speed;
    int              duplex;
};

struct phy_device *phy_probe(struct mdio_bus *bus, uint8_t addr);
int  phy_init(struct phy_device *phy);
int  phy_update_link(struct phy_device *phy);

#endif /* VINIX_PHY_H */
