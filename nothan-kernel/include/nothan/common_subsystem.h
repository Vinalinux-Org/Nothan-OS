/*
 * include/nothan/common_subsystem.h — common output subsystem.
 */

#ifndef NOTHAN_COMMON_SUBSYSTEM_H
#define NOTHAN_COMMON_SUBSYSTEM_H

#include "types.h"

void common_subsystem_putc(char ch);
int common_subsystem_write(const void *buf, uint32_t len);
void common_subsystem_write_string(const char *s);

#endif /* NOTHAN_COMMON_SUBSYSTEM_H */
