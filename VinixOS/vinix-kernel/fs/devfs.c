/* ============================================================
 * devfs.c
 * ------------------------------------------------------------
 * Adapter exposing struct cdev / file_operations through VFS.
 * Builds an ephemeral struct file per call (no inode model yet).
 * ============================================================ */

#include "devfs.h"
#include "vinix/cdev.h"
#include "vinix/fs.h"
#include "uart.h"
#include "syscalls.h"
#include "wait_queue.h"
#include "string.h"

/* ------------------------------------------------------------
 * /dev/null — silently discards writes, returns EOF on read.
 * ------------------------------------------------------------ */
static int null_read(struct file *f, void *buf, uint32_t len)
{
    (void)f; (void)buf; (void)len;
    return 0;
}

static int null_write(struct file *f, const void *buf, uint32_t len)
{
    (void)f; (void)buf;
    return (int)len;
}

static const struct file_operations null_fops = {
    .read  = null_read,
    .write = null_write,
};

/* ------------------------------------------------------------
 * /dev/tty — wraps the UART. Read blocks on uart_rx_wq; write
 * expands \n → \r\n like the legacy sys_write path.
 * ------------------------------------------------------------ */
static int tty_read(struct file *f, void *buf, uint32_t len)
{
    (void)f;
    if (len == 0) return 0;

    char *out = buf;
    wait_event(uart_rx_wq, uart_rx_available() > 0);

    int c = uart_getc();
    if (c < 0) return 0;
    out[0] = (char)c;
    return 1;
}

static int tty_write(struct file *f, const void *buf, uint32_t len)
{
    (void)f;
    const char *s = buf;
    for (uint32_t i = 0; i < len; i++) {
        if (s[i] == '\n') uart_putc('\r');
        uart_putc(s[i]);
    }
    return (int)len;
}

static const struct file_operations tty_fops = {
    .read  = tty_read,
    .write = tty_write,
};

/* ------------------------------------------------------------
 * VFS adapter — dispatches via fops with an ephemeral struct file.
 * file_index identifies the cdev slot in the registry.
 * ------------------------------------------------------------ */
static int devfs_lookup(const char *name)
{
    return cdev_find(name);
}

static int devfs_read(int idx, uint32_t offset, void *buf, uint32_t len)
{
    const struct cdev *c = cdev_at((uint32_t)idx);
    if (!c || !c->fops || !c->fops->read) return E_FAIL;

    struct file f = {
        .f_op         = c->fops,
        .private_data = c->priv,
        .f_pos        = offset,
        .f_flags      = 0,
    };
    return c->fops->read(&f, buf, len);
}

static int devfs_write(int idx, uint32_t offset, const void *buf, uint32_t len)
{
    const struct cdev *c = cdev_at((uint32_t)idx);
    if (!c || !c->fops || !c->fops->write) return E_PERM;

    struct file f = {
        .f_op         = c->fops,
        .private_data = c->priv,
        .f_pos        = offset,
        .f_flags      = 0,
    };
    return c->fops->write(&f, buf, len);
}

static int devfs_get_file_count(void)
{
    return cdev_count();
}

static int devfs_get_file_info(int index, char *name_out, uint32_t *size_out)
{
    const struct cdev *c = cdev_at((uint32_t)index);
    if (!c) return E_NOENT;

    int i = 0;
    while (c->name[i] && i < 31) { name_out[i] = c->name[i]; i++; }
    name_out[i] = '\0';
    if (size_out) *size_out = 0;
    return E_OK;
}

static int devfs_listdir(const char *path, void *entries, uint32_t max)
{
    /* devfs is flat — the only valid path is the mount root. */
    if (path && *path != '\0') return E_NOENT;

    file_info_t *out = (file_info_t *)entries;
    uint32_t written = 0;
    int total = cdev_count();

    for (int i = 0; i < total && written < max; i++) {
        const struct cdev *c = cdev_at((uint32_t)i);
        if (!c) continue;

        int k = 0;
        while (c->name[k] && k < 31) { out[written].name[k] = c->name[k]; k++; }
        out[written].name[k] = '\0';
        out[written].size = 0;
        written++;
    }
    return (int)written;
}

static struct vfs_operations devfs_ops = {
    .lookup         = devfs_lookup,
    .read           = devfs_read,
    .get_file_count = devfs_get_file_count,
    .get_file_info  = devfs_get_file_info,
    .listdir        = devfs_listdir,
    .create         = 0,
    .write          = devfs_write,
    .truncate       = 0,
};

static const struct cdev null_cdev = {
    .name = "null",
    .fops = &null_fops,
    .priv = 0,
};

static const struct cdev tty_cdev = {
    .name = "tty",
    .fops = &tty_fops,
    .priv = 0,
};

struct vfs_operations *devfs_init(void)
{
    cdev_register(&tty_cdev);
    cdev_register(&null_cdev);
    pr_info("[DEVFS] registered /dev/tty, /dev/null\n");
    return &devfs_ops;
}
