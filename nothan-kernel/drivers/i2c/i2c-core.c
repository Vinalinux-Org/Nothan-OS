/*
 * drivers/i2c/i2c-core.c - I2C subsystem core
 *
 * Manages adapters, clients, and drivers. Matches clients to drivers
 * by name.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/i2c.h>
#include <nothan/printk.h>

#define MAX_ADAPTERS    3
#define MAX_CLIENTS     8
#define MAX_DRIVERS     8
#define MAX_BOARD_INFO  8

struct i2c_client_slot {
	struct i2c_client client;
	int               in_use;
};

static struct i2c_adapter    *adapters[MAX_ADAPTERS];
static struct i2c_driver     *drivers[MAX_DRIVERS];
static int                    n_drivers;

static struct i2c_board_info  board_info[MAX_ADAPTERS][MAX_BOARD_INFO];
static int                    board_info_n[MAX_ADAPTERS];

static struct i2c_client_slot clients[MAX_ADAPTERS * MAX_CLIENTS];

static struct i2c_client *alloc_client(void)
{
	for (int i = 0; i < MAX_ADAPTERS * MAX_CLIENTS; i++) {
		if (!clients[i].in_use) {
			clients[i].in_use = 1;
			return &clients[i].client;
		}
	}
	return NULL;
}

static int name_eq(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return *a == '\0' && *b == '\0';
}

static struct i2c_driver *find_driver(const char *name)
{
	for (int i = 0; i < n_drivers; i++) {
		if (drivers[i] && name_eq(drivers[i]->name, name))
			return drivers[i];
	}
	return NULL;
}

static void copy_name(char *dst, const char *src)
{
	int i = 0;
	while (i < I2C_NAME_SIZE - 1 && src[i]) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
}

static void instantiate_clients(struct i2c_adapter *adap)
{
	int bus = adap->nr;
	int n   = board_info_n[bus];

	for (int i = 0; i < n; i++) {
		struct i2c_client *client = alloc_client();
		if (!client) {
			printk("[I2C] no free client slot for bus %d\n", bus);
			continue;
		}

		copy_name(client->name, board_info[bus][i].name);
		client->addr    = board_info[bus][i].addr;
		client->adapter = adap;

		printk("[I2C] bus%d: client \"%s\" @ 0x%02x\n",
		       bus, client->name, client->addr);

		struct i2c_driver *drv = find_driver(client->name);
		if (drv) {
			int ret = drv->probe(client);
			if (ret < 0)
				printk("[I2C] %s probe failed (%d)\n",
				       client->name, ret);
		}
	}
}

/**
 * i2c_register_board_info - register devices present on an I2C bus
 * @bus:  bus number (0–2)
 * @info: array of device descriptors
 * @n:    number of entries
 *
 * Must be called before i2c_add_adapter() for the same bus.
 */
int i2c_register_board_info(int bus, const struct i2c_board_info *info,
			    int n)
{
	if (bus < 0 || bus >= MAX_ADAPTERS || n <= 0)
		return -1;

	if (board_info_n[bus] + n > MAX_BOARD_INFO)
		n = MAX_BOARD_INFO - board_info_n[bus];

	for (int i = 0; i < n; i++)
		board_info[bus][board_info_n[bus] + i] = info[i];

	board_info_n[bus] += n;
	return 0;
}

/**
 * i2c_add_adapter - register an I2C hardware adapter
 * @adap: adapter to register
 *
 * Instantiates all clients registered for this bus and probes drivers.
 */
int i2c_add_adapter(struct i2c_adapter *adap)
{
	int bus = adap->nr;

	if (bus < 0 || bus >= MAX_ADAPTERS) {
		printk("[I2C] invalid bus number %d\n", bus);
		return -1;
	}
	if (adapters[bus]) {
		printk("[I2C] bus%d already registered\n", bus);
		return -1;
	}

	adapters[bus] = adap;
	printk("[I2C] adapter \"%s\" registered on bus%d\n",
	       adap->name, bus);

	instantiate_clients(adap);
	return 0;
}

/**
 * i2c_add_driver - register an I2C device driver
 * @driver: driver to register
 *
 * Probes the driver against all already-instantiated clients.
 */
int i2c_add_driver(struct i2c_driver *driver)
{
	if (n_drivers >= MAX_DRIVERS) {
		printk("[I2C] driver table full\n");
		return -1;
	}

	drivers[n_drivers++] = driver;

	for (int i = 0; i < MAX_ADAPTERS * MAX_CLIENTS; i++) {
		if (!clients[i].in_use)
			continue;

		struct i2c_client *c = &clients[i].client;
		if (!name_eq(driver->name, c->name))
			continue;

		int ret = driver->probe(c);
		if (ret < 0)
			printk("[I2C] %s probe failed (%d)\n",
			       driver->name, ret);
	}
	return 0;
}

/**
 * i2c_master_send - write bytes to a client
 */
int i2c_master_send(struct i2c_client *client, const u8 *buf, int count)
{
	struct i2c_msg msg = {
		.addr  = client->addr,
		.flags = 0,
		.len   = (u16)count,
		.buf   = (u8 *)buf,
	};
	return client->adapter->xfer(client->adapter, &msg, 1);
}

/**
 * i2c_master_recv - read bytes from a client
 */
int i2c_master_recv(struct i2c_client *client, u8 *buf, int count)
{
	struct i2c_msg msg = {
		.addr  = client->addr,
		.flags = I2C_M_RD,
		.len   = (u16)count,
		.buf   = buf,
	};
	return client->adapter->xfer(client->adapter, &msg, 1);
}

/**
 * i2c_smbus_write_byte_data - write one register byte
 */
int i2c_smbus_write_byte_data(struct i2c_client *client, u8 reg, u8 val)
{
	u8 buf[2] = { reg, val };
	return i2c_master_send(client, buf, 2);
}

/**
 * i2c_smbus_read_byte_data - read one register byte
 */
int i2c_smbus_read_byte_data(struct i2c_client *client, u8 reg, u8 *val)
{
	int ret = i2c_master_send(client, &reg, 1);
	if (ret < 0)
		return ret;
	return i2c_master_recv(client, val, 1);
}
