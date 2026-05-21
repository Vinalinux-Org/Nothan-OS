/*
 * lib/include/types.h — basic type definitions
 *
 * Delegates to the compiler's freestanding headers (arm-none-eabi ships
 * stdint/stdbool/stddef even for -ffreestanding builds) so that LVGL's
 * own inclusion of those headers never causes "conflicting types" errors.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>   /* uint8_t … uint64_t, int8_t … int64_t, uintptr_t, intptr_t */
#include <stdbool.h>  /* bool, true, false */
#include <stddef.h>   /* size_t, NULL */

/* ssize_t is not in the freestanding standard headers */
#ifndef _SSIZE_T_DEFINED
typedef signed int ssize_t;
#define _SSIZE_T_DEFINED
#endif

#endif /* TYPES_H */
