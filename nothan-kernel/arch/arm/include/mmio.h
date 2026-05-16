/*
 * arch/arm/include/mmio.h — memory-mapped I/O accessors
 *
 * Volatile read/write wrappers for 8, 16, and 32-bit register accesses.
 * The volatile qualifier prevents the compiler from caching or reordering
 * peripheral register reads and writes.
 */

#ifndef MMIO_H
#define MMIO_H

#include "types.h"

static inline void mmio_write32(uint32_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static inline uint32_t mmio_read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write16(uint32_t addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
}

static inline uint16_t mmio_read16(uint32_t addr)
{
    return *(volatile uint16_t *)addr;
}

static inline void mmio_write8(uint32_t addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
}

static inline uint8_t mmio_read8(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

#endif /* MMIO_H */