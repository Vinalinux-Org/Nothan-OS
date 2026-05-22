/*
 * arch/arm/kernel/svc_handler.c — system call dispatcher
 *
 * Called from exception_entry_svc after the SVC instruction is taken.
 * The calling convention is: r7 = syscall number, r0–r3 = arguments.
 * On return, svc_context.r0 is set to the syscall result and the
 * trampoline (svc_exit_trampoline) copies it back to the user's r0.
 *
 * validate_user_pointer() rejects any pointer that does not fall
 * entirely within the user virtual address range defined by USER_SPACE_VA
 * and USER_SPACE_MB.
 */

#include "types.h"
#include "assert.h"
#include "trace.h"
#include "scheduler.h"
#include "nothan/common_subsystem.h"
#include "syscalls.h"
#include "mmu.h"
#include "vfs.h"
#include "string.h"
#include "proc.h"
#include "svc_context.h"
#include "platform_device.h"
#include "nothan/tty.h"

extern uint8_t _shell_payload_start;
extern uint8_t _shell_payload_end;

static int validate_user_pointer(const void *ptr, uint32_t len)
{
    uint32_t start = (uint32_t)ptr;
    uint32_t end = start + len;

    if (end < start)
        return E_PTR;

    uint32_t allowed_start = USER_SPACE_VA;
    uint32_t allowed_end = USER_SPACE_VA + (USER_SPACE_MB * 1024 * 1024);

    if (start >= allowed_start && end <= allowed_end)
        return E_OK;

    return E_PTR;
}

/* ABI compatibility: legacy userspace passes write(buf, len) in r0,r1;
 * compiler runtime passes write(fd, buf, count) in r0,r1,r2. */
static int32_t sys_write(struct svc_context *ctx)
{
    const void *buf = NULL;
    uint32_t len = 0;
    int fd = 1;

    if (validate_user_pointer((const void *)ctx->r0, (uint32_t)ctx->r1) == E_OK)
    {
        buf = (const void *)ctx->r0;
        len = (uint32_t)ctx->r1;
    }
    else if (validate_user_pointer((const void *)ctx->r1, (uint32_t)ctx->r2) == E_OK)
    {
        fd = (int)ctx->r0;
        buf = (const void *)ctx->r1;
        len = (uint32_t)ctx->r2;
    }
    else
    {
        pr_info("[SVC] Security Violation: Invalid Ptr 0x%08x\n", (uint32_t)ctx->r0);
        return E_PTR;
    }

    if (fd != 1 && fd != 2)
        return E_ARG;

    if (len > 256)
        return E_ARG;

    return common_subsystem_write(buf, len);
}

static int32_t sys_exit(struct svc_context *ctx)
{
    int32_t status = (int32_t)ctx->r0;

    pr_info("[SVC] Task %d exiting with status %d\n", current->id, status);

    /* PID 1 must never leave the scheduler — restart from embedded payload
     * so the system keeps ticking even if init bugs out or exits on purpose. */
    if (current && current->pid == 1)
    {
        uint32_t payload_size = (uint32_t)&_shell_payload_end - (uint32_t)&_shell_payload_start;
        uint8_t *src = &_shell_payload_start;
        uint8_t *dst = (uint8_t *)USER_SPACE_VA;

        for (uint32_t i = 0; i < payload_size; i++) dst[i] = src[i];

        ctx->r0 = 0;
        ctx->r1 = 0;
        ctx->r2 = 0;
        ctx->r3 = 0;
        ctx->lr = USER_SPACE_VA;

        pr_info("[SVC] init (pid 1) restarted from payload\n");
        return E_OK;
    }

    do_exit(status);
    return 0;
}

static int32_t sys_yield(struct svc_context *ctx)
{
    /* Set need_reschedule BEFORE schedule() — without this, a voluntary yield
     * may be silently ignored if the flag was already cleared, leaving the
     * task spinning in a busy-wait loop instead of giving up the CPU. */
    extern volatile bool need_reschedule;
    need_reschedule = true;

    schedule();
    return E_OK;
}

/* ABI compatibility: legacy userspace passes read(buf, len) in r0,r1;
 * compiler runtime passes read(fd, buf, count) in r0,r1,r2. */
static int32_t sys_read(struct svc_context *ctx)
{
    void *buf = NULL;
    uint32_t len = 0;
    int fd = 0;

    if (validate_user_pointer((void *)ctx->r0, (uint32_t)ctx->r1) == E_OK)
    {
        buf = (void *)ctx->r0;
        len = (uint32_t)ctx->r1;
    }
    else if (validate_user_pointer((void *)ctx->r1, (uint32_t)ctx->r2) == E_OK)
    {
        fd = (int)ctx->r0;
        buf = (void *)ctx->r1;
        len = (uint32_t)ctx->r2;
    }
    else
    {
        return E_PTR;
    }

    if (fd != 0)
        return E_ARG;

    int val_result = validate_user_pointer(buf, len);
    if (val_result != E_OK)
    {
        pr_info("[SYS_READ] Validation FAILED: buf=0x%08x, len=%u, err=%d\n",
                    (uint32_t)buf, len, val_result);
        return E_PTR;
    }

    if (len == 0)
        return 0;

    /* Only len=1 is supported for the interactive shell path. */
    char *c_buf = (char *)buf;

    int c = tty_read_char();
    if (c == -1)
        return 0;

    *c_buf = (char)c;
    return 1;
}

static int32_t sys_get_tasks(struct svc_context *ctx)
{
    void *buf = (void *)ctx->r0;
    uint32_t max_count = (uint32_t)ctx->r1;

    uint32_t size = max_count * sizeof(process_info_t);
    if (validate_user_pointer(buf, size) != E_OK)
        return E_PTR;

    return scheduler_get_tasks(buf, max_count);
}

extern uint8_t _text_start[];
extern uint8_t _text_end[];
extern uint8_t _data_start[];
extern uint8_t _data_end[];
extern uint8_t _bss_start[];
extern uint8_t _bss_end[];
extern uint8_t _stack_start[];
extern uint8_t _svc_stack_top[];
extern uint8_t _kernel_end[];

static int32_t sys_get_meminfo(struct svc_context *ctx)
{
    mem_info_t *buf = (mem_info_t *)ctx->r0;

    if (validate_user_pointer(buf, sizeof(mem_info_t)) != E_OK)
        return E_PTR;

    buf->total = PLATFORM_DDR_SIZE_MB * 1024 * 1024;

    buf->kernel_text = (uint32_t)_text_end - (uint32_t)_text_start;
    buf->kernel_data = (uint32_t)_data_end - (uint32_t)_data_start;
    buf->kernel_bss = (uint32_t)_bss_end - (uint32_t)_bss_start;
    buf->kernel_stack = (uint32_t)_svc_stack_top - (uint32_t)_stack_start;

    uint32_t kernel_end = (uint32_t)_kernel_end;
    buf->free = buf->total - (kernel_end - PLATFORM_DDR_PA_BASE);

    return E_OK;
}

static int32_t sys_open(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    int flags = (int)ctx->r1;

    if (validate_user_pointer(path, MAX_PATH) != E_OK)
        return E_PTR;

    return vfs_open(path, flags);
}

static int32_t sys_read_file(struct svc_context *ctx)
{
    int fd = (int)ctx->r0;
    void *buf = (void *)ctx->r1;
    uint32_t len = (uint32_t)ctx->r2;

    if (validate_user_pointer(buf, len) != E_OK)
        return E_PTR;

    return vfs_read(fd, buf, len);
}

static int32_t sys_write_file(struct svc_context *ctx)
{
    int fd = (int)ctx->r0;
    const void *buf = (const void *)ctx->r1;
    uint32_t len = (uint32_t)ctx->r2;

    if (validate_user_pointer(buf, len) != E_OK)
        return E_PTR;

    return vfs_write(fd, buf, len);
}

static int32_t sys_close(struct svc_context *ctx)
{
    int fd = (int)ctx->r0;
    return vfs_close(fd);
}

static int32_t sys_listdir(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    file_info_t *entries = (file_info_t *)ctx->r1;
    uint32_t max_entries = (uint32_t)ctx->r2;

    if (validate_user_pointer(path, MAX_PATH) != E_OK)
        return E_PTR;

    uint32_t entries_size = max_entries * sizeof(file_info_t);
    if (validate_user_pointer(entries, entries_size) != E_OK)
        return E_PTR;

    return vfs_listdir(path, entries, max_entries);
}

static int32_t sys_exec(struct svc_context *ctx)
{
    const char *path = (const char *)ctx->r0;
    char **argv = (char **)ctx->r1;

    if (validate_user_pointer(path, MAX_PATH) != E_OK)
        return E_PTR;

    /* argv is validated inside do_exec (it may be NULL). */
    int rc = do_exec(path, ctx, argv);
    if (rc < 0) return rc;

    /* do_exec sets ctx->r0 = argc and ctx->lr to the new entry point;
     * return argc so the dispatcher's ctx->r0 = result is a no-op. */
    return (int32_t)ctx->r0;
}

/* ABI: r7 = syscall number, r0–r3 = arguments, return value → r0. */
void svc_handler(struct svc_context *ctx)
{
    uint32_t syscall_num = ctx->r7;
    int32_t result = E_INVAL;

    static uint32_t svc_call_count = 0;
    svc_call_count++;

    switch (syscall_num)
    {
    case SYS_WRITE:      result = sys_write(ctx);      break;
    case SYS_EXIT:       result = sys_exit(ctx);       break;
    case SYS_YIELD:      result = sys_yield(ctx);      break;
    case SYS_READ:       result = sys_read(ctx);       break;
    case SYS_GET_TASKS:  result = sys_get_tasks(ctx);  break;
    case SYS_GET_MEMINFO:result = sys_get_meminfo(ctx);break;
    case SYS_OPEN:       result = sys_open(ctx);       break;
    case SYS_READ_FILE:  result = sys_read_file(ctx);  break;
    case SYS_CLOSE:      result = sys_close(ctx);      break;
    case SYS_LISTDIR:    result = sys_listdir(ctx);    break;
    case SYS_EXEC:       result = sys_exec(ctx);       break;
    case SYS_WRITE_FILE: result = sys_write_file(ctx); break;

    case SYS_GETPID: {
        struct task_struct *t = current;
        result = t ? t->pid : -1;
        break;
    }

    case SYS_GETPPID: {
        struct task_struct *t = current;
        result = t ? t->ppid : -1;
        break;
    }

    case SYS_FORK:
        result = do_fork(ctx);
        break;

    case SYS_DUP:
        result = vfs_dup((int)ctx->r0);
        break;

    case SYS_UNLINK: {
        const char *path = (const char *)ctx->r0;
        if (validate_user_pointer(path, MAX_PATH) != E_OK) { result = E_PTR; break; }
        result = vfs_unlink(path);
        break;
    }

    case SYS_RENAME: {
        const char *old_p = (const char *)ctx->r0;
        const char *new_p = (const char *)ctx->r1;
        if (validate_user_pointer(old_p, MAX_PATH) != E_OK ||
            validate_user_pointer(new_p, MAX_PATH) != E_OK) { result = E_PTR; break; }
        result = vfs_rename(old_p, new_p);
        break;
    }

    case SYS_DEVLIST: {
        void *buf = (void *)ctx->r0;
        uint32_t max_count = (uint32_t)ctx->r1;
        uint32_t size = max_count * sizeof(dev_info_t);
        if (validate_user_pointer(buf, size) != E_OK) { result = E_PTR; break; }
        result = platform_list_devices(buf, max_count);
        break;
    }

    case SYS_DUP2:
        result = vfs_dup2((int)ctx->r0, (int)ctx->r1);
        break;

    case SYS_KILL: {
        int pid = (int)ctx->r0;
        int sig = (int)ctx->r1;
        /* MVP: any kill delivers SIGKILL. Exit status = 128 + sig for POSIX feel. */
        int exit_code = (sig > 0) ? (128 + sig) : 137;
        result = do_kill_by_pid(pid, exit_code);
        break;
    }

    case SYS_WAIT: {
        int st = 0;
        int pid = do_wait(&st);
        if (pid >= 0 && ctx->r0 != 0)
        {
            int *user_status = (int *)ctx->r0;
            if (validate_user_pointer(user_status, sizeof(int)) == E_OK)
                *user_status = st;
        }
        result = pid;
        break;
    }

    default:
        pr_err("[SVC] ERROR: Unknown Syscall %d\n", syscall_num);
        result = E_INVAL;
        break;
    }

    ctx->r0 = result;
}
