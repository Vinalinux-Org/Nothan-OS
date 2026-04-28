#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "types.h"

/* ============================================================
 * System Call Numbers (ABI Contract)
 * ============================================================ */
#define SYS_WRITE 0
#define SYS_EXIT 1
#define SYS_YIELD 2
#define SYS_READ 3
#define SYS_GET_TASKS 4
#define SYS_GET_MEMINFO 5
#define SYS_OPEN 6
#define SYS_READ_FILE 7
#define SYS_CLOSE 8
#define SYS_LISTDIR 9
#define SYS_EXEC 10
#define SYS_WRITE_FILE 11
#define SYS_FORK 12
#define SYS_WAIT 13
#define SYS_GETPID 14
#define SYS_GETPPID 15
#define SYS_KILL 16
#define SYS_DUP 17
#define SYS_DUP2 18
#define SYS_DEVLIST 19
#define SYS_UNLINK  20
#define SYS_RENAME  21

/* ============================================================
 * Data Structures
 * ============================================================ */

typedef struct
{
    uint32_t id;
    char name[32];
    uint32_t state;
} process_info_t;

typedef struct
{
    uint32_t total; // Total RAM (e.g. 128MB)
    uint32_t free;  // Approximate free heap
    uint32_t kernel_text;
    uint32_t kernel_data;
    uint32_t kernel_bss;
    uint32_t kernel_stack;
} mem_info_t;

typedef struct
{
    char name[32]; // File name
    uint32_t size; // File size in bytes
} file_info_t;

typedef struct
{
    char     name[16];    // Device name (e.g. "omap-uart")
    uint32_t base;        // IORESOURCE_MEM start, 0 if none
    int      irq;         // IORESOURCE_IRQ start, -1 if none
    char     driver[16];  // Bound driver name, empty if unbound
} dev_info_t;

/* ============================================================
 * Error Codes
 * ============================================================ */
#define E_OK 0     /* Success */
#define E_FAIL -1  /* Generic failure */
#define E_INVAL -2 /* Invalid syscall number */
#define E_ARG -3   /* Invalid argument */
#define E_PTR -4   /* Invalid pointer (User Memory Rule) */
#define E_PERM -5  /* Permission denied */
#define E_NOENT -6 /* No such file or directory */
#define E_BADF -7  /* Bad file descriptor */
#define E_MFILE -8 /* Too many open files */

/* ============================================================
 * File Open Flags
 * ============================================================ */
#define O_RDONLY 0x00 /* Read only */
#define O_WRONLY 0x01 /* Write only */
#define O_RDWR   0x02 /* Read/Write */
#define O_ACCMODE 0x03 /* Access mode mask */
#define O_CREAT  0x04 /* Create file if missing */
#define O_TRUNC  0x08 /* Truncate file to zero length on open */
#define O_APPEND 0x10 /* Seek to end-of-file on open (for >> redirect) */

#endif /* _SYSCALLS_H */
