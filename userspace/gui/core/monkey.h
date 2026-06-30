#ifndef __GUI_MONKEY_H
#define __GUI_MONKEY_H

#include <stdint.h>

void monkey_init(uint32_t seed);
void monkey_read(int *lx, int *ly, int *pressed);

#endif
