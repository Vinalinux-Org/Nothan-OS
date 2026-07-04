#ifndef _NOTHAN_MEMORY_H
#define _NOTHAN_MEMORY_H

#define PAGE_OFFSET	0xC0000000UL
#define PHYS_OFFSET	0x80000000UL

/* Kernel lives at phys 0x80000000 → virt 0xC0000000. */
#define __phys_to_virt(x)	((x) + PAGE_OFFSET - PHYS_OFFSET)
#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)

#endif /* _NOTHAN_MEMORY_H */
