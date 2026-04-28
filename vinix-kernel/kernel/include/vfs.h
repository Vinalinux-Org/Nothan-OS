/* ============================================================
 * vfs.h
 * ------------------------------------------------------------
 * Virtual File System Interface
 * ============================================================ */

#ifndef VFS_H
#define VFS_H

#include "types.h"

/* ============================================================
 * Constants
 * ============================================================ */
#define MAX_FDS         16      /* Maximum open file descriptors */
#define MAX_PATH        256     /* Maximum path length */

/* Per-process file-descriptor entry. Lives in task_struct.files[].
 * fs_ops points at whichever filesystem served the open() — so read
 * and write dispatch to the right backend instead of a hard-coded "/". */
struct vfs_ops;
struct vfs_fd {
    bool                   in_use;
    uint32_t               file_index;
    uint32_t               offset;
    int                    flags;
    struct vfs_operations *fs_ops;
};

/* ============================================================
 * VFS Operations Structure
 * ============================================================ */

struct vfs_operations {
    /* Read path */
    int (*lookup)(const char *name);
    int (*read)(int file_index, uint32_t offset, void *buf, uint32_t len);
    int (*get_file_count)(void);
    int (*get_file_info)(int index, char *name_out, uint32_t *size_out);

    /* Enumerate a directory — called with an fs-relative path.
     * Empty path means the mount's root. Optional. */
    int (*listdir)(const char *path, void *entries, uint32_t max);

    /* Write path — optional; NULL means filesystem is read-only */
    int (*create)(const char *name);
    int (*write)(int file_index, uint32_t offset, const void *buf, uint32_t len);
    int (*truncate)(int file_index, uint32_t new_size);
    int (*unlink)(const char *name);
    int (*rename)(const char *old_name, const char *new_name);
};

/* ============================================================
 * VFS Initialization
 * ============================================================ */

void vfs_init(void);

/* All vfs_* return negative errno on failure. */
int vfs_mount(const char *mount_point, struct vfs_operations *fs_ops);

/* ============================================================
 * File Operations
 * ============================================================ */

int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buf, uint32_t len);
int vfs_write(int fd, const void *buf, uint32_t len);
int vfs_close(int fd);

/* Duplicate fd into the lowest available slot. Returns new fd or error. */
int vfs_dup(int oldfd);

/* Point newfd at the same underlying file as oldfd, closing newfd first. */
int vfs_dup2(int oldfd, int newfd);

int vfs_unlink(const char *path);
int vfs_rename(const char *old_path, const char *new_path);

int vfs_listdir(const char *path, void *entries, uint32_t max_entries);

#endif /* VFS_H */
