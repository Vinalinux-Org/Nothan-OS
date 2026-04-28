/* ============================================================
 * commands.c
 * ------------------------------------------------------------
 * Built-in Shell Commands (User Mode)
 * ============================================================ */

#include "shell.h"
#include "types.h"

/* Map internal logging to shell helpers within Shell Task */
extern int printf(const char *fmt, ...);
void shell_putc(char c);
void shell_puts(const char *s);
#define uart_putc shell_putc
#define uart_puts shell_puts
#define uart_printf printf

#include "syscalls.h"
#include "user_syscall.h"
#include "string.h"

/* ============================================================
 * Command Implementations
 * ============================================================ */

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    extern const struct command cmd_table[];
    const struct command *cmd = cmd_table;

    printf("\nAvailable Commands:\n");
    printf("-------------------\n");
    while (cmd->name)
    {
        printf("  %s\t: %s\n", cmd->name, cmd->description);
        cmd++;
    }
    printf("\n");
    return 0;
}

static int cmd_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("\nVinixOS Info:\n");
    printf("  OS Name  : VinixOS\n");
    printf("  Platform : BeagleBone Black (Cortex-A8)\n");
    printf("  Arch     : ARMv7-A\n");
    printf("  Developer: Vinalinux <vinalinux2022@gmail.com>\n");
    printf("\n");
    return 0;
}

static int cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* ANSI Escape Sequence for Clear Screen */
    printf("\033[2J\033[H");
    return 0;
}

static int cmd_ps(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    process_info_t tasks[16];
    int count = sys_get_tasks(tasks, 16);

    if (count < 0)
    {
        printf("Error getting tasks\n");
        return -1;
    }

    printf("\n%-8s%-32s%-16s\n", "ID", "NAME", "STATE");
    printf("%-8s%-32s%-16s\n", "--", "----", "-----");
    for (int i = 0; i < count; i++)
    {
        const char *state_str = "UNKNOWN";
        switch (tasks[i].state)
        {
        case 0:
            state_str = "READY";
            break;
        case 1:
            state_str = "RUNNING";
            break;
        case 2:
            state_str = "BLOCKED";
            break;
        case 3:
            state_str = "ZOMBIE";
            break;
        }

        /* Formatting: Left-aligned fixed width columns */
        printf("%-8d%-32s%-16s\n",
               tasks[i].id, tasks[i].name, state_str);
    }
    printf("\n");
    return 0;
}

static int cmd_mem(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    mem_info_t info;
    if (sys_get_meminfo(&info) != 0)
    {
        printf("Error getting mem info\n");
        return -1;
    }

    printf("\nMemory Layout (128MB Total):\n");
    printf("  Kernel Text : %u bytes\n", info.kernel_text);
    printf("  Kernel Data : %u bytes\n", info.kernel_data);
    printf("  Kernel BSS  : %u bytes\n", info.kernel_bss);
    printf("  Kernel Stack: %u bytes\n", info.kernel_stack);
    printf("  Free Memory : %u bytes\n", info.free);
    printf("\n");
    return 0;
}

static int cmd_ls(int argc, char **argv)
{
    const char *path = "/";
    if (argc > 1)
    {
        path = argv[1];
    }

    file_info_t files[16];
    int count = sys_listdir(path, files, 16);

    if (count < 0)
    {
        printf("Error listing directory: %d\n", count);
        return -1;
    }

    for (int i = 0; i < count; i++)
    {
        printf("  %s\n", files[i].name);
    }
    return 0;
}

static int cmd_cat(int argc, char **argv)
{
    /* No arg: stream from shell_stdin_fd (set by `<` redirect).
     * Without a redirect, shell_stdin_fd is 0 — the kernel stdin —
     * which would block on UART, so require either an arg or a
     * redirect. */
    int fd;
    bool opened_here = false;

    if (argc >= 2) {
        const char *filename = argv[1];
        fd = sys_open(filename, O_RDONLY);
        if (fd < 0) {
            printf("Error opening file '%s': %d\n", filename, fd);
            return -1;
        }
        opened_here = true;
    } else if (shell_stdin_fd > 2) {
        fd = shell_stdin_fd;
    } else {
        printf("Usage: cat <filename>   (or redirect: cat < file)\n");
        return -1;
    }

    char buffer[256];
    int bytes_read;

    printf("\n");

    while ((bytes_read = sys_read_file(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }

    if (bytes_read < 0)
    {
        printf("\nError reading file: %d\n", bytes_read);
    }

    printf("\n");

    if (opened_here) sys_close(fd);
    return 0;
}

static int cmd_write(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: write <filename> <text...>\n");
        printf("Example: write hello.txt Hello World\n");
        return -1;
    }

    const char *filename = argv[1];

    int fd = sys_open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        printf("Error opening '%s' for write: %d\n", filename, fd);
        return -1;
    }

    int total_written = 0;
    for (int i = 2; i < argc; i++)
    {
        if (i > 2)
        {
            sys_write_file(fd, " ", 1);
            total_written += 1;
        }
        int n = sys_write_file(fd, argv[i], (uint32_t)strlen(argv[i]));
        if (n < 0)
        {
            printf("Write failed: %d\n", n);
            sys_close(fd);
            return -1;
        }
        total_written += n;
    }

    sys_write_file(fd, "\n", 1);
    total_written += 1;

    sys_close(fd);
    printf("Wrote %d bytes to %s\n", total_written, filename);
    return 0;
}

static int cmd_pid(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("pid=%d ppid=%d\n", sys_getpid(), sys_getppid());
    return 0;
}

static int cmd_fork(int argc, char **argv)
{
    (void)argc; (void)argv;
    int pid = sys_fork();
    if (pid < 0) {
        printf("fork failed: %d\n", pid);
        return -1;
    }
    if (pid == 0) {
        /* Child path: announce then exit so parent's wait() reaps. */
        printf("[child] pid=%d ppid=%d — exiting\n", sys_getpid(), sys_getppid());
        sys_exit(42);
        return 0;  /* unreachable */
    }
    /* Parent: wait for child. */
    int status = 0;
    int w = sys_wait(&status);
    printf("[parent] child pid=%d exited with status=%d\n", w, status);
    return 0;
}

static int cmd_kill(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: kill <pid>\n");
        return -1;
    }
    int pid = 0;
    for (const char *p = argv[1]; *p >= '0' && *p <= '9'; p++) {
        pid = pid * 10 + (*p - '0');
    }
    int r = sys_kill(pid, SIGKILL);
    if (r < 0) {
        printf("kill %d: cannot kill (slot empty, self, or protected)\n", pid);
        return -1;
    }
    /* Reap if the target was one of our children. */
    int status = 0;
    int w = sys_wait(&status);
    if (w >= 0) {
        printf("reaped pid=%d status=%d\n", w, status);
    }
    return 0;
}

static int cmd_devlist(int argc, char **argv)
{
    (void)argc; (void)argv;
    dev_info_t devs[8];
    int n = sys_devlist(devs, 8);
    if (n < 0) {
        printf("devlist failed: %d\n", n);
        return -1;
    }
    printf("\n%-14s%-12s%-6s%-14s\n", "NAME", "BASE", "IRQ", "DRIVER");
    printf("%-14s%-12s%-6s%-14s\n", "----", "----", "---", "------");
    for (int i = 0; i < n; i++) {
        char irqbuf[8];
        if (devs[i].irq < 0) {
            irqbuf[0] = '-'; irqbuf[1] = '\0';
        } else {
            /* small int-to-decimal */
            int v = devs[i].irq, k = 0;
            char rev[6];
            if (v == 0) rev[k++] = '0';
            while (v > 0) { rev[k++] = '0' + (v % 10); v /= 10; }
            for (int j = 0; j < k; j++) irqbuf[j] = rev[k - 1 - j];
            irqbuf[k] = '\0';
        }
        printf("%-14s0x%08x  %-6s%s\n",
               devs[i].name, devs[i].base, irqbuf,
               devs[i].driver[0] ? devs[i].driver : "(unbound)");
    }
    printf("\n");
    return 0;
}

static int cmd_crash(int argc, char **argv)
{
    (void)argc; (void)argv;
    int pid = sys_fork();
    if (pid < 0) {
        printf("fork failed: %d\n", pid);
        return -1;
    }
    if (pid == 0) {
        /* Child crashes with NULL dereference — parent must survive. */
        printf("[child] pid=%d about to NULL-deref...\n", sys_getpid());
        volatile int *p = (volatile int *)0;
        *p = 0xdead;
        sys_exit(0);  /* unreachable */
        return 0;
    }
    int status = 0;
    int w = sys_wait(&status);
    printf("[parent] child pid=%d died with status=%d (139 = SIGSEGV)\n", w, status);
    return 0;
}

static int cmd_exec(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: exec <path>\n");
        printf("Example: exec /bin/ls\n");
        return -1;
    }

    const char *filename = argv[1];

    printf("Executing '%s'...\n", filename);

    int ret = sys_exec(filename, 0);
    if (ret < 0)
    {
        printf("exec failed for '%s': %d\n", filename, ret);
        return -1;
    }

    /* Should never return on success */
    printf("exec returned unexpectedly: %d\n", ret);
    return -1;
}

/* ============================================================
 * Command Table
 * ============================================================ */
const struct command cmd_table[] = {
    {"help", cmd_help, "help", "Show this help message"},
    {"info", cmd_info, "info", "Show system info"},
    {"ps", cmd_ps, "ps", "Show running tasks"},
    {"mem", cmd_mem, "mem", "Show memory usage"},
    {"ls", cmd_ls, "ls [path]", "List directory contents"},
    {"cat", cmd_cat, "cat <file>", "Display file contents"},
    {"write", cmd_write, "write <file> <text>", "Write text to file (create/truncate)"},
    {"exec", cmd_exec, "exec <file>", "Execute binary file"},
    {"echo", cmd_echo, "echo [args]", "Echo arguments"},
    {"clear", cmd_clear, "clear", "Clear screen"},
    {"pid", cmd_pid, "pid", "Print current pid + ppid"},
    {"fork", cmd_fork, "fork", "Fork a child that exits immediately"},
    {"kill", cmd_kill, "kill <pid>", "Kill a running task by pid"},
    {"crash", cmd_crash, "crash", "Fork a child that NULL-derefs (demo isolation)"},
    {"devlist", cmd_devlist, "devlist", "Show platform bus devices + bound drivers"},
    {NULL, NULL, NULL, NULL}};