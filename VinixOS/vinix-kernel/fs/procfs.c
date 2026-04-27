/* ============================================================
 * procfs.c — virtual /proc. Content is generated on read from
 * live kernel state; nothing is cached on disk.
 *
 * File index encoding:
 *   0              /proc/meminfo
 *   1              /proc/version
 *   2              /proc/mounts
 *   3..3+N-1       /proc/<slot>/status (N = MAX_TASKS)
 * ============================================================ */

#include "procfs.h"
#include "vfs.h"
#include "scheduler.h"
#include "task.h"
#include "syscalls.h"
#include "string.h"
#include "format.h"
#include "page_alloc.h"
#include "buffer_cache.h"
#include "uart.h"

#define PROCFS_STATIC_FILES 3
#define PROCFS_STATUS_BASE  3

/* Kernel symbols used in meminfo. */
extern uint8_t _text_start[], _text_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

#define PROCFS_GEN_BUF 512

static char gen_buf[PROCFS_GEN_BUF];

/* ------------------------------------------------------------
 * Generators — each writes into gen_buf and returns length.
 * Returning 0 means empty / no content; negative means error.
 * ------------------------------------------------------------ */

static int gen_meminfo(void)
{
    uint32_t text = (uint32_t)(_text_end - _text_start);
    uint32_t data = (uint32_t)(_data_end - _data_start);
    uint32_t bss  = (uint32_t)(_bss_end  - _bss_start);

    uint32_t total_pages = page_alloc_total_pages();
    uint32_t free_pages  = page_alloc_free_pages();
    return ksnprintf(gen_buf, PROCFS_GEN_BUF,
        "MemTotal:      %10u kB\n"
        "PoolTotal:     %10u kB\n"
        "PoolFree:      %10u kB\n"
        "KernelText:    %10u bytes\n"
        "KernelData:    %10u bytes\n"
        "KernelBSS:     %10u bytes\n"
        "BufCacheHits:  %10u\n"
        "BufCacheMiss:  %10u\n",
        128u * 1024u,
        total_pages * 4u,
        free_pages  * 4u,
        text, data, bss,
        bcache_hits(), bcache_misses());
}

static int gen_version(void)
{
    return ksnprintf(gen_buf, PROCFS_GEN_BUF,
        "VinixOS 0.1 (armv7-a) BeagleBone Black\n"
        "Built with arm-none-eabi-gcc\n"
        "100%% hand-written — kernel, libc, userspace\n");
}

static int gen_mounts(void)
{
    /* Hardcoded mirror of main.c's mount order. Procfs doesn't yet
     * enumerate the VFS mount table — keep it static for MVP. */
    return ksnprintf(gen_buf, PROCFS_GEN_BUF,
        "/      fat32\n"
        "/dev   devfs\n"
        "/proc  procfs\n");
}

static const char *state_str(uint32_t s)
{
    switch (s) {
    case TASK_RUNNING:        return "R";
    case TASK_INTERRUPTIBLE:  return "S";
    case TASK_ZOMBIE:         return "Z";
    default:                  return "?";
    }
}

static int gen_pid_status(uint32_t slot)
{
    struct task_struct *t = tasks_array_get(slot);
    if (!t) return 0;

    return ksnprintf(gen_buf, PROCFS_GEN_BUF,
        "Name:         %s\n"
        "Pid:          %d\n"
        "PPid:         %d\n"
        "State:        %s\n"
        "ExitStatus:   %d\n"
        "StackBase:    0x%08x\n"
        "StackSize:    %u bytes\n"
        "Pgd:          0x%08x\n",
        t->name ? t->name : "(unnamed)",
        t->pid, t->ppid,
        state_str(t->state),
        t->exit_status,
        (uint32_t)t->stack_base,
        t->stack_size,
        t->pgd_pa);
}

/* Regenerate gen_buf for the given index. Returns length. */
static int regenerate(int file_index)
{
    if (file_index == 0) return gen_meminfo();
    if (file_index == 1) return gen_version();
    if (file_index == 2) return gen_mounts();

    int slot = file_index - PROCFS_STATUS_BASE;
    if (slot >= 0 && slot < (int)MAX_TASKS) return gen_pid_status((uint32_t)slot);

    return 0;
}

/* ------------------------------------------------------------
 * VFS adapter
 * ------------------------------------------------------------ */

/* Split "X/Y" into (X, Y). Returns Y or NULL if no slash. */
static const char *split_component(const char *path, char *first, uint32_t cap)
{
    uint32_t i = 0;
    while (*path && *path != '/' && i + 1 < cap) {
        first[i++] = *path++;
    }
    first[i] = '\0';
    if (*path == '/') return path + 1;
    return 0;
}

static int parse_pid(const char *s, int *pid_out)
{
    int v = 0;
    int any = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
        any = 1;
    }
    if (!any || *s != '\0') return -1;
    *pid_out = v;
    return 0;
}

static int procfs_lookup(const char *name)
{
    if (!name || !*name) return E_NOENT;

    /* Static files at the root. */
    if (strcmp(name, "meminfo") == 0) return 0;
    if (strcmp(name, "version") == 0) return 1;
    if (strcmp(name, "mounts")  == 0) return 2;

    /* "<pid>/status" — per-process introspection. */
    char head[8];
    const char *rest = split_component(name, head, sizeof(head));
    if (!rest) return E_NOENT;

    int pid;
    if (parse_pid(head, &pid) != 0) return E_NOENT;
    if (pid < 0 || pid >= (int)MAX_TASKS) return E_NOENT;
    if (!tasks_array_get((uint32_t)pid)) return E_NOENT;
    if (strcmp(rest, "status") != 0) return E_NOENT;

    return PROCFS_STATUS_BASE + pid;
}

static int procfs_read(int file_index, uint32_t offset, void *buf, uint32_t len)
{
    int n = regenerate(file_index);
    if (n <= 0) return 0;
    if (offset >= (uint32_t)n) return 0;
    uint32_t avail = (uint32_t)n - offset;
    uint32_t take  = len < avail ? len : avail;
    memcpy(buf, gen_buf + offset, take);
    return (int)take;
}

static int procfs_listdir(const char *path, void *entries, uint32_t max)
{
    file_info_t *out = (file_info_t *)entries;
    uint32_t count = 0;

    if (!path || !*path) {
        const char *names[] = { "meminfo", "version", "mounts" };
        for (int i = 0; i < 3 && count < max; i++) {
            strcpy(out[count].name, names[i]);
            out[count].size = 0;
            count++;
        }
        for (uint32_t slot = 0; slot < MAX_TASKS && count < max; slot++) {
            if (!tasks_array_get(slot)) continue;
            ksnprintf(out[count].name, sizeof(out[count].name), "%u", slot);
            out[count].size = 0;
            count++;
        }
        return (int)count;
    }

    /* Subdirectory listing: "<pid>" → "status". */
    int pid;
    if (parse_pid(path, &pid) == 0 &&
        pid >= 0 && pid < (int)MAX_TASKS &&
        tasks_array_get((uint32_t)pid))
    {
        if (count < max) {
            strcpy(out[count].name, "status");
            out[count].size = 0;
            count++;
        }
        return (int)count;
    }

    return E_NOENT;
}

static int procfs_get_file_count(void)
{
    int count = PROCFS_STATIC_FILES;
    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        if (tasks_array_get(i)) count++;
    }
    return count;
}

static int procfs_get_file_info(int index, char *name_out, uint32_t *size_out)
{
    if (index < 0) return E_BADF;

    static const char *statics[] = { "meminfo", "version", "mounts" };
    if (index < 3) {
        strcpy(name_out, statics[index]);
        if (size_out) *size_out = 0;
        return E_OK;
    }

    int slot = index - PROCFS_STATUS_BASE;
    if (slot < 0 || slot >= (int)MAX_TASKS) return E_BADF;
    if (!tasks_array_get((uint32_t)slot)) return E_BADF;

    ksnprintf(name_out, 32, "%u", slot);
    if (size_out) *size_out = 0;
    return E_OK;
}

static struct vfs_operations procfs_ops = {
    .lookup         = procfs_lookup,
    .read           = procfs_read,
    .get_file_count = procfs_get_file_count,
    .get_file_info  = procfs_get_file_info,
    .listdir        = procfs_listdir,
    .create         = 0,
    .write          = 0,
    .truncate       = 0,
};

struct vfs_operations *procfs_init(void)
{
    static bool announced = false;
    if (!announced) {
        pr_info("[PROCFS] registered /proc\n");
        announced = true;
    }
    return &procfs_ops;
}
