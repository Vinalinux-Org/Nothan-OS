/* ============================================================
 * vfs.c
 * ------------------------------------------------------------
 * Virtual File System — mount table + per-task fd table.
 * ============================================================ */

#include "vfs.h"
#include "syscalls.h"
#include "uart.h"
#include "string.h"
#include "types.h"
#include "task.h"
#include "scheduler.h"

/* Bootstrap fd table — used before the scheduler hands back a
 * current task. Real tasks use task_struct.files[]. */
static struct vfs_fd boot_fd_table[MAX_FDS];

static struct vfs_fd *current_fds(void)
{
    struct task_struct *t = current;
    return t ? t->files : boot_fd_table;
}

struct vfs_mount {
    const char            *mount_point;
    struct vfs_operations *fs_ops;
    int                    mp_len;  /* cached strlen(mount_point) */
    bool                   in_use;
};

#define MAX_MOUNTS 4
static struct vfs_mount mount_table[MAX_MOUNTS];

void vfs_init(void)
{
    pr_info("[VFS] Initializing Virtual File System...\n");

    for (int i = 0; i < MAX_FDS; i++) {
        boot_fd_table[i].in_use = false;
        boot_fd_table[i].file_index = 0;
        boot_fd_table[i].offset = 0;
        boot_fd_table[i].flags = 0;
        boot_fd_table[i].fs_ops = NULL;
    }

    for (int i = 0; i < MAX_MOUNTS; i++) {
        mount_table[i].mount_point = NULL;
        mount_table[i].fs_ops = NULL;
        mount_table[i].mp_len = 0;
        mount_table[i].in_use = false;
    }

    pr_info("[VFS] Initialization complete\n");
}

int vfs_mount(const char *mount_point, struct vfs_operations *fs_ops)
{
    pr_info("[VFS] Mounting filesystem at '%s'...\n", mount_point);

    int slot = -1;
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].in_use) { slot = i; break; }
    }
    if (slot < 0) {
        pr_err("[VFS] ERROR: No free mount slots\n");
        return E_FAIL;
    }

    mount_table[slot].mount_point = mount_point;
    mount_table[slot].fs_ops = fs_ops;
    mount_table[slot].mp_len = strlen(mount_point);
    mount_table[slot].in_use = true;

    pr_info("[VFS] Filesystem mounted at '%s'\n", mount_point);
    return E_OK;
}

/* Return the longest matching mount. Writes the mount-relative
 * path to *rest_out (never with a leading slash).
 *
 * Paths without a leading '/' are treated as relative to the '/'
 * mount — matches shell ergonomics (users type "HELLO.TXT" not
 * "/HELLO.TXT"). */
static struct vfs_operations *resolve_root(const char **rest_out)
{
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mount_table[i].in_use && mount_table[i].mp_len == 1 &&
            mount_table[i].mount_point[0] == '/') {
            if (rest_out) *rest_out = "";
            return mount_table[i].fs_ops;
        }
    }
    return NULL;
}

static struct vfs_operations *resolve(const char *path, const char **rest_out)
{
    if (path[0] != '/') {
        struct vfs_operations *ops = resolve_root(rest_out);
        if (ops && rest_out) *rest_out = path;
        return ops;
    }

    int best = -1;
    int best_len = -1;

    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].in_use) continue;
        const char *mp = mount_table[i].mount_point;
        int mp_len = mount_table[i].mp_len;

        if (strncmp(path, mp, (size_t)mp_len) != 0) continue;

        /* The mount boundary must line up with a separator so
         * "/devices" doesn't match a "/dev" mount. */
        char next = path[mp_len];
        bool boundary_ok = (mp[mp_len - 1] == '/') || next == '/' || next == '\0';
        if (!boundary_ok) continue;

        if (mp_len > best_len) { best_len = mp_len; best = i; }
    }

    if (best < 0) { if (rest_out) *rest_out = NULL; return NULL; }

    const char *rest = path + best_len;
    while (*rest == '/') rest++;
    if (rest_out) *rest_out = rest;
    return mount_table[best].fs_ops;
}

int vfs_open(const char *path, int flags)
{
    int access = flags & O_ACCMODE;
    if (access != O_RDONLY && access != O_WRONLY && access != O_RDWR) {
        return E_ARG;
    }

    const char *rest;
    struct vfs_operations *fs_ops = resolve(path, &rest);
    if (!fs_ops) return E_NOENT;

    if ((access == O_WRONLY || access == O_RDWR) && fs_ops->write == NULL) {
        return E_PERM;
    }

    int file_index = fs_ops->lookup(rest);
    if (file_index < 0) {
        if ((flags & O_CREAT) && fs_ops->create != NULL) {
            file_index = fs_ops->create(rest);
            if (file_index < 0) return file_index;
        } else {
            return E_NOENT;
        }
    } else if (flags & O_TRUNC) {
        if (fs_ops->truncate != NULL) {
            int r = fs_ops->truncate(file_index, 0);
            if (r < 0) return r;
        }
    }

    struct vfs_fd *fds = current_fds();
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (!fds[i].in_use) { fd = i; break; }
    }
    if (fd < 0) return E_MFILE;

    uint32_t start_offset = 0;
    if ((flags & O_APPEND) && fs_ops->get_file_info != NULL) {
        char dummy[32];
        uint32_t sz = 0;
        if (fs_ops->get_file_info(file_index, dummy, &sz) == E_OK) {
            start_offset = sz;
        }
    }

    fds[fd].in_use     = true;
    fds[fd].file_index = file_index;
    fds[fd].offset     = start_offset;
    fds[fd].flags      = flags;
    fds[fd].fs_ops     = fs_ops;
    return fd;
}

int vfs_read(int fd, void *buf, uint32_t len)
{
    struct vfs_fd *fds = current_fds();
    if (fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return E_BADF;

    struct vfs_operations *fs_ops = fds[fd].fs_ops;
    if (!fs_ops || !fs_ops->read) return E_FAIL;

    int n = fs_ops->read(fds[fd].file_index, fds[fd].offset, buf, len);
    if (n > 0) fds[fd].offset += n;
    return n;
}

int vfs_write(int fd, const void *buf, uint32_t len)
{
    struct vfs_fd *fds = current_fds();
    if (fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return E_BADF;

    int access = fds[fd].flags & O_ACCMODE;
    if (access != O_WRONLY && access != O_RDWR) return E_PERM;

    struct vfs_operations *fs_ops = fds[fd].fs_ops;
    if (!fs_ops || !fs_ops->write) return E_PERM;

    int n = fs_ops->write(fds[fd].file_index, fds[fd].offset, buf, len);
    if (n > 0) fds[fd].offset += n;
    return n;
}

int vfs_close(int fd)
{
    struct vfs_fd *fds = current_fds();
    if (fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return E_BADF;

    fds[fd].in_use = false;
    fds[fd].file_index = 0;
    fds[fd].offset = 0;
    fds[fd].flags = 0;
    fds[fd].fs_ops = NULL;
    return E_OK;
}

int vfs_dup(int oldfd)
{
    struct vfs_fd *fds = current_fds();
    if (oldfd < 0 || oldfd >= MAX_FDS || !fds[oldfd].in_use) return E_BADF;
    for (int i = 3; i < MAX_FDS; i++) {
        if (!fds[i].in_use) {
            fds[i] = fds[oldfd];
            return i;
        }
    }
    return E_MFILE;
}

int vfs_dup2(int oldfd, int newfd)
{
    struct vfs_fd *fds = current_fds();
    if (oldfd < 0 || oldfd >= MAX_FDS || !fds[oldfd].in_use) return E_BADF;
    if (newfd < 0 || newfd >= MAX_FDS) return E_BADF;
    if (oldfd == newfd) return newfd;

    /* Drop whatever was in newfd silently — POSIX semantics. */
    fds[newfd] = fds[oldfd];
    return newfd;
}

int vfs_unlink(const char *path)
{
    const char *rest;
    struct vfs_operations *fs_ops = resolve(path, &rest);
    if (!fs_ops || !fs_ops->unlink) return E_PERM;
    return fs_ops->unlink(rest);
}

int vfs_rename(const char *old_path, const char *new_path)
{
    const char *old_rest;
    const char *new_rest;
    struct vfs_operations *old_fs = resolve(old_path, &old_rest);
    struct vfs_operations *new_fs = resolve(new_path, &new_rest);
    if (!old_fs || !new_fs) return E_NOENT;
    if (old_fs != new_fs) return E_PERM;  /* cross-mount rename not supported */
    if (!old_fs->rename) return E_PERM;
    return old_fs->rename(old_rest, new_rest);
}

int vfs_listdir(const char *path, void *entries, uint32_t max_entries)
{
    const char *rest;
    struct vfs_operations *fs_ops = resolve(path, &rest);
    if (!fs_ops) return E_NOENT;

    if (fs_ops->listdir) {
        return fs_ops->listdir(rest ? rest : "", entries, max_entries);
    }

    /* Fallback for filesystems that still expose the legacy
     * get_file_count + get_file_info API (root-only). */
    if (rest && *rest != '\0') return E_NOENT;
    if (!fs_ops->get_file_count || !fs_ops->get_file_info) return E_NOENT;

    file_info_t *out = (file_info_t *)entries;
    int total = fs_ops->get_file_count();
    if (total < 0) return total;

    int count = 0;
    for (int i = 0; i < total && count < (int)max_entries; i++) {
        if (fs_ops->get_file_info(i, out[count].name, &out[count].size) == E_OK) {
            count++;
        }
    }
    return count;
}
