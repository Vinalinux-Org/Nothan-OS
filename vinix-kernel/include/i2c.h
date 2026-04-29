/*
 * include/i2c.h — AM335x I2C0 driver interface
 */

#ifndef I2C_H
#define I2C_H

#include "types.h"



/* I2C0 base address (L4_WKUP domain, already mapped) */
#define I2C0_BASE           0x44E0B000



/* Returns -1 on NACK or timeout. */
int i2c_read_reg(uint8_t slave_addr, uint8_t reg, uint8_t *val);
int i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val);

/* Sends `reg` then reads `len` bytes — for EDID and multi-byte regs. */
int i2c_read_block(uint8_t slave_addr, uint8_t reg, uint8_t *buf, int len);

#endif /* I2C_H */
