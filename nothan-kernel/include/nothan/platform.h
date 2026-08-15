#ifndef _NOTHAN_PLATFORM_H
#define _NOTHAN_PLATFORM_H

#include <nothan/device.h>

struct platform_device {
	struct device dev;
	const char   *name;
	u32           base;
	int           irq;
};

struct platform_driver {
	struct driver drv;
	int (*probe)(struct platform_device *pdev);
	int (*remove)(struct platform_device *pdev);
};

int platform_device_register(struct platform_device *pdev);
int platform_driver_register(struct platform_driver *pdrv);
struct resource *platform_get_resource(struct platform_device *pdev,
				       u32 type, unsigned int index);
int platform_get_irq(struct platform_device *pdev, unsigned int index);

/*
 * The board's own Ethernet MAC address, six bytes.
 *
 * Board data, not driver data: where the address comes from is a property of
 * the board (a fuse here, possibly an EEPROM or a stored setting elsewhere),
 * while the driver only needs the six bytes.  CLAUDE.md keeps that boundary,
 * and it is the boundary that lets the same driver run on the product board.
 */
void board_get_mac_addr(u8 *addr);

/* Platform bus — global, defined in drivers/base/platform.c */
extern struct bus_type platform_bus;

#endif /* _NOTHAN_PLATFORM_H */
