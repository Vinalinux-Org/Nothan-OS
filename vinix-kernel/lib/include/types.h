/*
 * lib/include/types.h — compiler-independent basic type definitions
 *
 * Guards prevent conflicts when LVGL pulls in compiler <stdint.h>/<stdbool.h>.
 * arm-none-eabi gcc defines uint32_t as 'long unsigned int'; we must not redefine.
 */

#ifndef TYPES_H
#define TYPES_H

/* Integer types — skip if <stdint.h> already declared them */
#ifndef __INT8_TYPE__

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

#endif /* __INT8_TYPE__ */

/* Size and pointer types */
#ifndef __SIZE_TYPE__
typedef unsigned int size_t;
#endif

#ifndef __PTRDIFF_TYPE__
typedef signed int ssize_t;
#endif

#ifndef __UINTPTR_TYPE__
typedef unsigned int uintptr_t;
#endif

#ifndef __INTPTR_TYPE__
typedef signed int intptr_t;
#endif

/* Boolean — skip if <stdbool.h> already defined true/false */
#ifndef __bool_true_false_are_defined
typedef enum { false = 0, true = 1 } bool;
#define __bool_true_false_are_defined 1
#endif

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* TYPES_H */
