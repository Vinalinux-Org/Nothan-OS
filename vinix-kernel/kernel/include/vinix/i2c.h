/* ============================================================
 * vinix/i2c.h
 * ------------------------------------------------------------
 * I2C subsystem — adapter (host controller), algorithm (xfer
 * function), client (slave attached on the bus).
 *
 * Adapter drivers (i2c-omap) fill struct i2c_adapter with an
 * i2c_algorithm and call i2c_add_adapter(). Client drivers
 * (tda998x) call i2c_transfer(adapter, msgs, count) instead of
 * touching the host registers directly.
 * ============================================================ */

#ifndef VINIX_I2C_H
#define VINIX_I2C_H

#include "types.h"

#define I2C_M_RD    0x0001    /* read direction */

struct i2c_msg {
    uint16_t  addr;     /* 7-bit slave address */
    uint16_t  flags;    /* I2C_M_* */
    uint16_t  len;
    uint8_t  *buf;
};

struct i2c_adapter;

struct i2c_algorithm {
    int (*master_xfer)(struct i2c_adapter *adap,
                       struct i2c_msg *msgs, int count);
};

struct i2c_adapter {
    const char                  *name;
    int                          nr;       /* bus number */
    const struct i2c_algorithm  *algo;
    void                        *priv;
};

struct i2c_client {
    struct i2c_adapter *adapter;
    uint16_t            addr;
    const char         *name;
    void               *priv;
};

int                 i2c_add_adapter(struct i2c_adapter *adap);
int                 i2c_transfer  (struct i2c_adapter *adap,
                                   struct i2c_msg *msgs, int count);
struct i2c_adapter *i2c_get_adapter(int nr);

#endif /* VINIX_I2C_H */
