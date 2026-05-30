#ifndef _MMIO_H
#define _MMIO_H

#include <nothan/types.h>

#define mmio_read32(a)		({ u32 __v = *(volatile u32 *)(a); __v; })
#define mmio_write32(a, v)	do { *(volatile u32 *)(a) = (v); } while (0)

#endif /* _MMIO_H */
