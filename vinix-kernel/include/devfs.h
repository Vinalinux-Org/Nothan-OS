/* ============================================================
 * devfs.h
 * ------------------------------------------------------------
 * /dev filesystem — exposes registered char_devices as files.
 * ============================================================ */

#ifndef DEVFS_H
#define DEVFS_H

#include "vfs.h"

/* Register built-in devices (null, tty) and return ops for mounting. */
struct vfs_operations *devfs_init(void);

#endif /* DEVFS_H */
