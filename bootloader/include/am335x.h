#ifndef AM335X_H
#define AM335X_H

#include "types.h"
#include "am335x_clock.h"
#include "am335x_uart.h"
#include "am335x_ddr.h"
#include "am335x_mmc.h"
#include "am335x_pinmux.h"

/* Memory map */
#define SRAM_BASE       0x402F0400
#define SRAM_SIZE       0x0001BC00  /* 109 KB */
#define DDR_BASE        0x80000000
#define DDR_SIZE        0x20000000  /* 512 MB */

/* Watchdog Timer 1 */
#define WDT1_BASE       0x44E35000
#define WDT1_WSPR       (WDT1_BASE + 0x48)
#define WDT1_WWPS       (WDT1_BASE + 0x34)

/* Register access */
#define REG32(addr)         (*(volatile uint32_t *)(addr))
#define REG16(addr)         (*(volatile uint16_t *)(addr))
#define REG8(addr)          (*(volatile uint8_t *)(addr))

#define readl(addr)         REG32(addr)
#define writel(val, addr)   (REG32(addr) = (val))
#define readw(addr)         REG16(addr)
#define writew(val, addr)   (REG16(addr) = (val))
#define readb(addr)         REG8(addr)
#define writeb(val, addr)   (REG8(addr) = (val))

/* Bit manipulation */
#define BIT(n)              (1U << (n))
#define SETBIT(reg, bit)    ((reg) |= BIT(bit))
#define CLRBIT(reg, bit)    ((reg) &= ~BIT(bit))
#define GETBIT(reg, bit)    (((reg) >> (bit)) & 1)

#endif /* AM335X_H */
