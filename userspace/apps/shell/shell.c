/* ============================================================
 * shell.c
 * ------------------------------------------------------------
 * User Mode Interactive Shell Task
 * ============================================================ */

#include "shell.h"
#include "user_syscall.h"
#include "syscalls.h"
#include "types.h"
#include "string.h"
#include <stdarg.h>

/* ============================================================
 * Configuration
 * ============================================================ */
#define SHELL_LINE_BUFFER_SIZE 128

int shell_stdin_fd  = 0;
int shell_stdout_fd = 1;

/* Map internal logging to shell helpers */
#define uart_putc shell_putc
#define uart_puts shell_puts
#define uart_printf printf

/* ============================================================
 * Output Helpers (Syscall Wrappers)
 * ============================================================ */

static void stdout_write(const void *buf, uint32_t len)
{
    if (shell_stdout_fd == 1) {
        sys_write(buf, len);
    } else {
        sys_write_file(shell_stdout_fd, buf, len);
    }
}

void shell_putc(char c)
{
    /* Ensure address is on stack */
    char buf = c;
    stdout_write(&buf, 1);
}

void shell_puts(const char *s)
{
    /*
     * Buffer on stack (valid user memory)
     * Copying string to stack bypasses validation checks
     * on .rodata and reduces syscall overhead.
     */
    char buf[64];
    int i = 0;

    while (*s)
    {
        /* Newline expansion only applies when writing to the kernel
         * stdout (UART). Redirected files shouldn't have \r injected. */
        if (*s == '\n' && shell_stdout_fd == 1)
        {
            if (i > 0) {
                stdout_write(buf, i);
                i = 0;
            }
            buf[0] = '\r';
            stdout_write(buf, 1);
        }

        buf[i++] = *s++;

        if (i >= 64)
        {
            stdout_write(buf, i);
            i = 0;
        }
    }

    if (i > 0)
    {
        stdout_write(buf, i);
    }
}

/* Format via vinixlibc's vsnprintf, then fan out through shell_puts
 * so shell_stdout_fd (redirect target) and the UART \n→\r\n
 * expansion are still honoured. Keeps one implementation of %d/%s/…
 * for every userspace consumer. */
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

int printf(const char *fmt, ...)
{
    char out[256];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(out, sizeof(out), fmt, args);
    va_end(args);
    shell_puts(out);
    return n;
}

/* ============================================================
 * Command Parser & Dispatcher
 * ============================================================ */

/* Forward declaration from commands.c */
extern const struct command cmd_table[];

static int is_redirect(const char *tok)
{
    return strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0 || strcmp(tok, "<") == 0;
}

/* Returns -1 on parse/open error, 0 on success. On success, *new_argc
 * holds the argv count with redirect tokens stripped, and the caller
 * must close redirect fds + restore shell_stdin_fd/stdout_fd afterwards. */
static int apply_redirects(char **args, int argc, int *new_argc,
                           int *stdin_fd_out, int *stdout_fd_out)
{
    int out_fd = -1, in_fd = -1;
    int w = 0;

    for (int i = 0; i < argc; i++) {
        if (!is_redirect(args[i])) { args[w++] = args[i]; continue; }

        if (i + 1 >= argc) {
            printf("syntax error: '%s' needs a filename\n", args[i]);
            return -1;
        }

        const char *target = args[i + 1];
        if (strcmp(args[i], "<") == 0) {
            int fd = sys_open(target, O_RDONLY);
            if (fd < 0) {
                printf("open '%s' for read failed: %d\n", target, fd);
                return -1;
            }
            if (in_fd >= 0) sys_close(in_fd);
            in_fd = fd;
        } else {
            int flags = O_WRONLY | O_CREAT;
            flags |= (strcmp(args[i], ">>") == 0) ? O_APPEND : O_TRUNC;
            int fd = sys_open(target, flags);
            if (fd < 0) {
                printf("open '%s' for write failed: %d\n", target, fd);
                if (in_fd >= 0) sys_close(in_fd);
                return -1;
            }
            if (out_fd >= 0) sys_close(out_fd);
            out_fd = fd;
        }
        i++;  /* consume the filename too */
    }

    *new_argc = w;
    *stdin_fd_out  = in_fd;
    *stdout_fd_out = out_fd;
    return 0;
}

static int execute_command(char *line)
{
    char *args[SHELL_MAX_ARGS];
    int argc = 0;
    char *cmd = line;
    char *p = line;

    /* 1. Skip leading whitespace */
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0')
        return 0; /* Empty line */

    cmd = p;

    /* 2. Tokenize */
    while (*p != '\0' && argc < SHELL_MAX_ARGS)
    {
        args[argc++] = p;

        /* Find end of token */
        while (*p != '\0' && *p != ' ' && *p != '\t')
            p++;

        if (*p == '\0')
            break;

        /* Terminate token */
        *p++ = '\0';

        /* Skip to next token */
        while (*p == ' ' || *p == '\t')
            p++;
    }

    /* 2.5. Extract redirects. Leaves args[] pointing at the command only. */
    int redir_in = -1, redir_out = -1;
    int stripped_argc = argc;
    if (apply_redirects(args, argc, &stripped_argc, &redir_in, &redir_out) != 0) {
        return -1;
    }
    if (stripped_argc == 0) {
        if (redir_in  >= 0) sys_close(redir_in);
        if (redir_out >= 0) sys_close(redir_out);
        return 0;
    }

    int saved_stdin  = shell_stdin_fd;
    int saved_stdout = shell_stdout_fd;
    if (redir_in  >= 0) shell_stdin_fd  = redir_in;
    if (redir_out >= 0) shell_stdout_fd = redir_out;

    cmd = args[0];
    int rc = -1;

    /* 3. Dispatch */
    const struct command *entry = cmd_table;
    while (entry->name != NULL)
    {
        if (strcmp(entry->name, cmd) == 0)
        {
            rc = entry->handler(stripped_argc, args);
            goto out;
        }
        entry++;
    }

    /* Not a built-in — fork + exec as external ELF. Child replaces
     * its image on success; parent waits for status so the shell
     * survives whatever the program does (including a crash). */
    int child_pid = sys_fork();
    if (child_pid < 0) {
        printf("fork failed: %d\n", child_pid);
    } else if (child_pid == 0) {
        /* Stage a NULL-terminated argv on the child's stack. The kernel
         * snapshots these pointers/strings before overwriting user
         * memory with the new ELF. */
        char *child_argv[SHELL_MAX_ARGS + 1];
        for (int i = 0; i < stripped_argc && i < SHELL_MAX_ARGS; i++) {
            child_argv[i] = args[i];
        }
        child_argv[stripped_argc] = 0;

        /* Bare command name → try /bin/<cmd> first, then the root
         * fallback so cards built before the FHS layout still work. */
        int er;
        bool has_slash = false;
        for (const char *p = cmd; *p; p++) {
            if (*p == '/') { has_slash = true; break; }
        }

        if (!has_slash) {
            char resolved[64] = "/bin/";
            int k = 5;
            for (int i = 0; cmd[i] && k < 63; i++) resolved[k++] = cmd[i];
            resolved[k] = '\0';
            er = sys_exec(resolved, child_argv);
        }
        er = sys_exec(cmd, child_argv);

        printf("'%s': exec failed (%d) — not a built-in, not a file\n",
               cmd, er);
        sys_exit(127);
    } else {
        int status = 0;
        sys_wait(&status);
        rc = status;
    }

out:
    shell_stdin_fd  = saved_stdin;
    shell_stdout_fd = saved_stdout;
    if (redir_in  >= 0) sys_close(redir_in);
    if (redir_out >= 0) sys_close(redir_out);
    return rc;
}

/* ============================================================
 * Line Buffer
 * ============================================================ */
static struct
{
    char data[SHELL_LINE_BUFFER_SIZE];
    uint32_t pos;
} line_buffer;

static void line_buffer_init(void)
{
    line_buffer.pos = 0;
    line_buffer.data[0] = '\0';
}

static int line_buffer_add(char c)
{
    if (line_buffer.pos >= SHELL_LINE_BUFFER_SIZE - 1)
        return -1;
    line_buffer.data[line_buffer.pos++] = c;
    line_buffer.data[line_buffer.pos] = '\0';
    return 0;
}

static int line_buffer_backspace(void)
{
    if (line_buffer.pos > 0)
    {
        line_buffer.pos--;
        line_buffer.data[line_buffer.pos] = '\0';
        return 0;
    }
    return -1;
}

/* ============================================================
 * Shell Main Entry
 * ============================================================ */
int main(void)
{
    char c;

    /* 1. Initialization */
    line_buffer_init();

    printf("\n");
    printf("========================================\n");
    printf(" VinixOS User Shell\n");
    printf("========================================\n");

    /* DEBUG: Check CPSR Mode */
    uint32_t cpsr;
    __asm__ __volatile__("mrs %0, cpsr" : "=r"(cpsr));
    printf("[SHELL] CPSR=0x%x, Mode=0x%x (USR=0x10, SVC=0x13)\n", cpsr, cpsr & 0x1F);

    printf("> ");

    /* 2. Main Loop */
    while (1)
    {
        /* Non-Blocking Read */
        int result = sys_read(&c, 1);

        if (result > 0)
        {
            /* Got data - process it */

            /* Handle Entry */
            if (c == '\r' || c == '\n')
            {
                shell_putc('\n');
                execute_command(line_buffer.data);
                line_buffer_init();
                printf("> ");
            }
            /* Handle Backspace (ASCII 8 or 127) */
            else if (c == 0x08 || c == 0x7F)
            {
                if (line_buffer_backspace() == 0)
                {
                    /* Visual Backspace */
                    shell_putc('\b');
                    shell_putc(' ');
                    shell_putc('\b');
                }
            }
            /* Handle Printable Chars */
            else if (c >= 32 && c <= 126)
            {
                if (line_buffer_add(c) == 0)
                {
                    shell_putc(c); /* Local Echo */
                }
                else
                {
                    shell_putc('\a'); /* Beep */
                }
            }
        }
        else
        {
            /* No data available - yield to other tasks */
            sys_yield();
        }
    }

    return 0;
}