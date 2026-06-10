/*
 * kernel/vfs/vfs.c - Virtual File System core
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/fs.h>
#include <nothan/printk.h>
#include <nothan/slab.h>

#define MAX_FDS 16
static struct file *fd_table[MAX_FDS];

extern int fat32_mount(struct super_block *sb);
struct super_block *root_sb = NULL;

/**
 * vfs_mount() - Mount a file system
 * @dev_name: Name of the block device to mount
 * @fs_type: Name of the file system type
 *
 * Return: 0 on success, < 0 on error
 */
int vfs_mount(const char *dev_name, const char *fs_type)
{
	struct block_device *bdev = get_block_device(dev_name);
	if (!bdev) {
		printk("[VFS] Cannot find block device '%s'\n", dev_name);
		return -1;
	}

	printk("[VFS] Mounting %s on %s\n", fs_type, dev_name);
	
	/* Allocate super_block */
	struct super_block *sb = (struct super_block *)kmalloc(sizeof(*sb), GFP_KERNEL);
	if (!sb)
		return -1;
	
	sb->s_bdev = bdev;
	sb->s_root = NULL;

	/* Simple fs type dispatch */
	if (fs_type[0] == 'f' && fs_type[1] == 'a' && fs_type[2] == 't') {
		if (fat32_mount(sb) != 0) {
			kfree(sb);
			return -1;
		}
	} else {
		printk("[VFS] Unsupported fs type '%s'\n", fs_type);
		kfree(sb);
		return -1;
	}

	root_sb = sb;
	return 0;
}

/**
 * vfs_open() - Open a file
 * @pathname: Path to the file
 * @flags: Open flags (O_RDONLY, etc.)
 *
 * Return: File descriptor (fd) >= 0 on success, < 0 on error
 */
int vfs_open(const char *pathname, int flags)
{
	int fd = -1;
	for (int i = 0; i < MAX_FDS; i++) {
		if (!fd_table[i]) {
			fd = i;
			break;
		}
	}

	if (fd < 0) {
		printk("[VFS] Out of file descriptors\n");
		return -1;
	}

	if (!root_sb || !root_sb->s_op || !root_sb->s_op->lookup_root) {
		printk("[VFS] No root filesystem available\n");
		return -1;
	}

	struct inode *inode = root_sb->s_op->lookup_root(root_sb, pathname);
	if (!inode)
		return -1;

	struct file *f = (struct file *)kmalloc(sizeof(*f), GFP_KERNEL);
	if (!f) {
		kfree(inode);
		return -1;
	}

	f->f_pos = 0;
	f->f_flags = flags;
	f->f_inode = inode;
	f->f_op = inode->i_fop;
	f->private_data = NULL;

	if (f->f_op && f->f_op->open) {
		if (f->f_op->open(inode, f) != 0) {
			kfree(f);
			kfree(inode);
			return -1;
		}
	}

	fd_table[fd] = f;
	return fd;
}

/**
 * vfs_read() - Read from a file descriptor
 * @fd: File descriptor
 * @buf: User buffer
 * @count: Number of bytes to read
 *
 * Return: Number of bytes read, < 0 on error
 */
int vfs_read(int fd, char *buf, size_t count)
{
	if (fd < 0 || fd >= MAX_FDS || !fd_table[fd])
		return -1;

	struct file *f = fd_table[fd];
	if (f->f_op && f->f_op->read)
		return f->f_op->read(f, buf, count);

	return 0;
}

/**
 * vfs_write() - Write to a file descriptor
 * @fd: File descriptor
 * @buf: User buffer
 * @count: Number of bytes to write
 *
 * Return: Number of bytes written, < 0 on error
 */
int vfs_write(int fd, const char *buf, size_t count)
{
	if (fd < 0 || fd >= MAX_FDS || !fd_table[fd])
		return -1;

	struct file *f = fd_table[fd];
	if (f->f_op && f->f_op->write)
		return f->f_op->write(f, buf, count);

	return -1;
}

/**
 * vfs_close() - Close a file descriptor
 * @fd: File descriptor
 *
 * Return: 0 on success, < 0 on error
 */
int vfs_close(int fd)
{
	if (fd < 0 || fd >= MAX_FDS || !fd_table[fd])
		return -1;

	struct file *f = fd_table[fd];
	if (f->f_op && f->f_op->release)
		f->f_op->release(f->f_inode, f);

	kfree(f);
	fd_table[fd] = NULL;
	return 0;
}
