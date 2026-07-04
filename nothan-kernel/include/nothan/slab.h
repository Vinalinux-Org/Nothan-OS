#ifndef _SLAB_H
#define _SLAB_H

#include <nothan/types.h>
#include <nothan/mm.h>

void slab_init(void);
void *kmalloc(size_t size, unsigned int flags);
void kfree(void *ptr);

#endif /* _SLAB_H */
