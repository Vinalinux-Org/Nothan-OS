/*
 * bus.c - Generic bus, device, driver infrastructure
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/device.h>
#include <nothan/printk.h>

/**
 * bus_register() - Initialize a bus type
 * @bus: The bus_type to register
 *
 * Return: 0 always.
 */
int bus_register(struct bus_type *bus)
{
	bus->num_devices = 0;
	bus->num_drivers = 0;
	return 0;
}

/**
 * bus_add_device() - Add a device to a bus and try to match a driver
 * @bus: The bus to add the device to
 * @dev: The device to add
 *
 * If a matching driver is already registered, calls its probe function.
 *
 * Return: 0 on success, -1 if the device table is full.
 */
int bus_add_device(struct bus_type *bus, struct device *dev)
{
	if (bus->num_devices >= BUS_MAX_DEVICES)
		return -1;

	dev->bus = bus;
	bus->devices[bus->num_devices++] = dev;

	/* Try to match with already-registered drivers. */
	for (int i = 0; i < bus->num_drivers; i++) {
		if (bus->match && bus->match(dev, bus->drivers[i])) {
			dev->driver = bus->drivers[i];
			if (dev->driver->probe)
				dev->driver->probe(dev);
			break;
		}
	}

	printk("[BUS] device '%s' added on %s bus\n", dev->name, bus->name);
	return 0;
}

/**
 * bus_add_driver() - Register a driver on a bus and probe matching devices
 * @bus: The bus to register the driver on
 * @drv: The driver to register
 *
 * Probes any device already on the bus that matches by name.
 *
 * Return: 0 on success, -1 if the driver table is full.
 */
int bus_add_driver(struct bus_type *bus, struct driver *drv)
{
	if (bus->num_drivers >= BUS_MAX_DRIVERS)
		return -1;

	drv->bus = bus;
	bus->drivers[bus->num_drivers++] = drv;

	/* Match against all already-registered devices. */
	for (int i = 0; i < bus->num_devices; i++) {
		if (bus->match && bus->match(bus->devices[i], drv)) {
			bus->devices[i]->driver = drv;
			if (drv->probe)
				drv->probe(bus->devices[i]);
			printk("[BUS] driver '%s' bound to '%s'\n",
			       drv->name, bus->devices[i]->name);
			break;
		}
	}

	return 0;
}
