#ifndef _NOTHAN_I2C_H
#define _NOTHAN_I2C_H

#include <nothan/types.h>

#define I2C_NAME_SIZE   16

/* Low-level transfer descriptor */
struct i2c_msg {
	u8   addr;
	u16  flags;
	u16  len;
	u8  *buf;
};

#define I2C_M_RD    0x0001  /* read direction */

/* Hardware adapter (one per I2C controller) */
struct i2c_adapter {
	char name[I2C_NAME_SIZE];
	int  nr;    /* bus number: 0, 1, 2 */
	int (*xfer)(struct i2c_adapter *adap,
		    struct i2c_msg *msgs, int num);
};

/* Device info provided by the board file */
struct i2c_board_info {
	char name[I2C_NAME_SIZE];
	u8   addr;
};

/* Instantiated device on a bus */
struct i2c_client {
	struct i2c_adapter *adapter;
	char  name[I2C_NAME_SIZE];
	u8    addr;
};

/* Device driver */
struct i2c_driver {
	const char *name;
	int (*probe)(struct i2c_client *client);
};

/*
 * Board-file API — call before i2c_add_adapter().
 * Stores device list so the adapter can instantiate clients on probe.
 */
int i2c_register_board_info(int bus, const struct i2c_board_info *info,
			    int n);

/* Adapter / driver registration */
int i2c_add_adapter(struct i2c_adapter *adap);
int i2c_add_driver(struct i2c_driver *driver);

/* Transfer helpers used by device drivers */
int i2c_master_send(struct i2c_client *client, const u8 *buf, int count);
int i2c_master_recv(struct i2c_client *client, u8 *buf, int count);
int i2c_smbus_write_byte_data(struct i2c_client *client, u8 reg, u8 val);
int i2c_smbus_read_byte_data(struct i2c_client *client, u8 reg, u8 *val);

#endif /* _NOTHAN_I2C_H */
