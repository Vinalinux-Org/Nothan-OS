/* ============================================================
 * procfs.h — virtual /proc filesystem.
 * ============================================================ */

#ifndef PROCFS_H
#define PROCFS_H

#include "vfs.h"

struct vfs_operations *procfs_init(void);

#endif
