/* ============================================================
 * i2c.h
 * ------------------------------------------------------------
 * AM335x I2C0 driver interface.
 * ============================================================ */

#ifndef I2C_H
#define I2C_H

#include "types.h"

/* ============================================================
 * I2C0 Configuration
 * ============================================================ */

/* I2C0 base address (L4_WKUP domain, already mapped) */
#define I2C0_BASE           0x44E0B000

/* ============================================================
 * Public API
 * ============================================================ */

/* ~100 kHz master polling mode.
 * Precondition: mmu_init() (peripheral mapping) + uart_init() done. */
void i2c_init(void);

/* Returns -1 on NACK or timeout. */
int i2c_read_reg(uint8_t slave_addr, uint8_t reg, uint8_t *val);
int i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val);

/* Sends `reg` then reads `len` bytes — for EDID and multi-byte regs. */
int i2c_read_block(uint8_t slave_addr, uint8_t reg, uint8_t *buf, int len);

/* Diagnostic — prints every responding address. */
void i2c_scan(void);

/* Register the I2C0 controller with the generic i2c-core as
 * adapter nr 0. Call after i2c_init(). */
void i2c_register_adapter(void);

#endif /* I2C_H */
