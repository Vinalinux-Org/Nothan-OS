/*
 * kernel/fs/devfs/devfs.c - In-memory virtual filesystem for /dev/
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * devfs is a read-only virtual filesystem mounted at "/dev/".
 * It has no backing storage — entries appear automatically when
 * a driver calls cdev_register(), and disappear on cdev_unregister().
 *
 * VFS integration:
 *   - vfs_mount("devfs", "devfs") sets up devfs_sb
 *   - walk_path() detects the "dev" component and switches to devfs_sb
 *   - listdir("/dev/") uses devfs_sb->s_op->readdir
 */

#include <nothan/fs.h>
#include <nothan/cdev.h>
#include <nothan/slab.h>
#include <nothan/printk.h>

/* Forward declarations */
static struct inode *devfs_alloc_inode(struct super_block *sb);
static void          devfs_destroy_inode(struct inode *inode);
static struct inode *devfs_dirlookup(struct inode *dir, const char *name);
static int           devfs_readdir(struct inode *dir, struct file_entry *buf, int max);

static const struct super_operations devfs_sops = {
	.alloc_inode    = devfs_alloc_inode,
	.destroy_inode  = devfs_destroy_inode,
	.dirlookup      = devfs_dirlookup,
	.readdir        = devfs_readdir,
};

/* Global devfs superblock — set by devfs_mount(), used by VFS */
struct super_block *devfs_sb = NULL;

/* Root directory inode for /dev/ */
static struct inode devfs_root_inode = {
	.i_ino   = 1,
	.i_size  = 0,
	.i_mode  = S_IFDIR,
	.i_fop   = NULL,
	.i_private = NULL,
};

static struct dentry devfs_root_dentry = {
	.d_name   = "dev",
	.d_inode  = &devfs_root_inode,
	.d_parent = NULL,
};

/**
 * devfs_alloc_inode - allocate a bare inode for a char device entry
 * @sb: devfs superblock
 *
 * The caller is responsible for setting i_mode, i_fop, and i_private.
 * Return: allocated inode, or NULL on allocation failure.
 */
static struct inode *devfs_alloc_inode(struct super_block *sb)
{
	struct inode *inode = kmalloc(sizeof(*inode), GFP_KERNEL);
	if (!inode)
		return NULL;
	inode->i_ino     = 0;
	inode->i_size    = 0;
	inode->i_mode    = S_IFCHR;
	inode->i_blocks  = 0;
	inode->i_sb      = sb;
	inode->i_fop     = NULL;
	inode->i_private = NULL;
	return inode;
}

static void devfs_destroy_inode(struct inode *inode)
{
	if (inode && inode != &devfs_root_inode)
		kfree(inode);
}

/**
 * devfs_dirlookup - look up a char device by name under /dev/
 * @dir:  /dev/ directory inode (ignored; devfs has a flat namespace)
 * @name: device name, e.g. "ttyS0", "i2c0"
 *
 * Return: newly allocated inode with i_fop set, or NULL if not found.
 */
static struct inode *devfs_dirlookup(struct inode *dir, const char *name)
{
	(void)dir;

	struct cdev *cd = cdev_lookup_by_name(name);
	if (!cd)
		return NULL;

	struct inode *inode = devfs_alloc_inode(devfs_sb);
	if (!inode)
		return NULL;

	inode->i_mode    = S_IFCHR;
	inode->i_fop     = cd->fops;
	inode->i_private = cd;
	return inode;
}

/**
 * devfs_readdir - enumerate all registered char devices
 * @dir: /dev/ directory inode (ignored)
 * @buf: output array of file_entry
 * @max: maximum entries to fill
 *
 * Return: number of entries filled.
 */
static int devfs_readdir(struct inode *dir, struct file_entry *buf, int max)
{
	(void)dir;
	int count = 0;

	for (int i = 0; i < CDEV_TABLE_SIZE && count < max; i++) {
		struct cdev *cd = cdev_lookup_by_index(i);
		if (!cd)
			continue;

		int j = 0;
		while (cd->name[j] && j < FILE_NAME_LEN - 1) {
			buf[count].name[j] = cd->name[j];
			j++;
		}
		buf[count].name[j] = '\0';
		buf[count].size = 0;
		count++;
	}
	return count;
}

/**
 * devfs_mount - set up the devfs superblock and attach it to the VFS
 *
 * Must be called after slab_init() and before any driver calls cdev_register().
 * Return: 0 on success, -1 on allocation failure.
 */
int devfs_mount(void)
{
	struct super_block *sb = kmalloc(sizeof(*sb), GFP_KERNEL);
	if (!sb)
		return -1;

	sb->s_bdev      = NULL;
	sb->s_op        = &devfs_sops;
	sb->s_root      = &devfs_root_dentry;
	sb->s_fs_info   = NULL;
	sb->s_blocksize = 0;

	devfs_root_dentry.d_sb    = sb;
	devfs_root_inode.i_sb     = sb;

	devfs_sb = sb;
	printk("[DEVFS] mounted at /dev/\n");
	return 0;
}
