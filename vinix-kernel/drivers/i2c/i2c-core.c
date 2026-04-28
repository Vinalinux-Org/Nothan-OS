/*
 * drivers/i2c/i2c-core.c — generic I2C bus core
 *
 * Host controller drivers register an i2c_adapter; client code calls
 * i2c_transfer() to send a sequence of i2c_msgs over a named bus.
 * Adapters are stored in a flat array indexed by adapter->nr.
 */

#include "vinix/i2c.h"
#include "vinix/errno.h"
#include "uart.h"
#include "string.h"

#define MAX_I2C_ADAPTERS 4

static struct i2c_adapter *adapters[MAX_I2C_ADAPTERS];
static int                 num_adapters = 0;

int i2c_add_adapter(struct i2c_adapter *adap)
{
    if (!adap || !adap->algo || !adap->algo->master_xfer) return -EINVAL;
    if (num_adapters >= MAX_I2C_ADAPTERS) return -ENOSPC;

    adap->nr = num_adapters;
    adapters[num_adapters++] = adap;
    pr_info("[I2C] adapter %d (%s) registered\n",
                adap->nr, adap->name ? adap->name : "?");
    return adap->nr;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int count)
{
    if (!adap || !adap->algo || !adap->algo->master_xfer) return -EINVAL;
    if (!msgs || count <= 0) return -EINVAL;

    return adap->algo->master_xfer(adap, msgs, count);
}

struct i2c_adapter *i2c_get_adapter(int nr)
{
    if (nr < 0 || nr >= num_adapters) return 0;
    return adapters[nr];
}
