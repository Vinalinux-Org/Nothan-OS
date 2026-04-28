/*
 * include/shell.h — Interactive shell interface and command table
 */

#ifndef SHELL_H
#define SHELL_H

#include "types.h"



/* Maximum line length (characters) */
#define SHELL_LINE_BUFFER_SIZE  128

/* Maximum number of arguments per command */
#define SHELL_MAX_ARGS          8

/* Maximum length of single argument */
#define SHELL_MAX_ARG_LEN       32





/* Main Entry Point for Shell Task */
void shell_task_entry(void);

/* Standard Output Wrapper */
void printf(const char *fmt, ...);

/* Helper Prototypes */
void shell_putc(char c);
void shell_puts(const char *s);


struct command {
    const char *name;
    int (*handler)(int argc, char **argv);
    const char *usage;
    const char *description;
};

#endif /* SHELL_H */