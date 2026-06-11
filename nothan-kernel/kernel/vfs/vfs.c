/*
 * kernel/vfs/vfs.c - Virtual File System core
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/fs.h>
#include <nothan/devfs.h>
#include <nothan/printk.h>
#include <nothan/slab.h>

#define MAX_FDS 16
static struct file *fd_table[MAX_FDS];

extern int fat32_mount(struct super_block *sb);
struct super_block *root_sb = NULL;

/**
 * vfs_mount() - Mount a file system
 * @dev_name: Name of the block device to mount
 * @fs_type: File system type (e.g., "fat32")
 *
 * Return: 0 on success, -1 on error.
 */
int vfs_mount(const char *dev_name, const char *fs_type)
{
	/* devfs has no block device — handle before get_block_device() */
	if (fs_type[0] == 'd' && fs_type[1] == 'e' && fs_type[2] == 'v')
		return devfs_mount();

	struct block_device *bdev = get_block_device(dev_name);
	if (!bdev) {
		printk("[VFS] Cannot find block device '%s'\n", dev_name);
		return -1;
	}

	printk("[VFS] Mounting %s on %s\n", fs_type, dev_name);

	struct super_block *sb = (struct super_block *)kmalloc(sizeof(*sb), GFP_KERNEL);
	if (!sb)
		return -1;

	sb->s_bdev = bdev;
	sb->s_root = NULL;

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
 * is_dev_prefix - check if path component starts with "dev"
 * @p: path with leading slashes already stripped
 *
 * Return: 1 if path is "dev" or starts with "dev/", 0 otherwise.
 */
static int is_dev_prefix(const char *p)
{
	return p[0] == 'd' && p[1] == 'e' && p[2] == 'v' &&
	       (p[3] == '/' || p[3] == '\0');
}

static struct inode *walk_path(const char *pathname)
{
	if (!root_sb || !root_sb->s_op || !root_sb->s_op->dirlookup)
		return NULL;

	/* Skip leading slashes */
	while (*pathname == '/')
		pathname++;

	/* Route /dev/<name> to devfs */
	if (devfs_sb && is_dev_prefix(pathname)) {
		const char *devname = pathname + 3;
		while (*devname == '/')
			devname++;
		if (*devname == '\0')
			return devfs_sb->s_root->d_inode; /* /dev/ itself */
		return devfs_sb->s_op->dirlookup(devfs_sb->s_root->d_inode, devname);
	}

	/* Root inode must be created by the FS during mount */
	if (!root_sb->s_root || !root_sb->s_root->d_inode)
		return NULL;

	struct inode *current = root_sb->s_root->d_inode;

	/* Walk each path component */
	char component[256];
	while (*pathname) {
		int len = 0;
		while (pathname[len] && pathname[len] != '/') {
			if (len < 255)
				component[len] = pathname[len];
			len++;
		}
		component[len] = '\0';

		struct inode *next = root_sb->s_op->dirlookup(current, component);
		if (!next)
			return NULL;

		current = next;
		pathname += len;
		while (*pathname == '/')
			pathname++;
	}

	return current;
}

/**
 * vfs_open() - Open a file
 * @pathname: Path to the file (e.g., "SHELL.BIN", "/bin/hello")
 * @flags: Open flags (O_RDONLY, O_WRONLY, etc.)
 *
 * Return: File descriptor (fd >= 0) on success, -1 on error.
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

	struct inode *inode = walk_path(pathname);
	if (!inode)
		return -1;

	/* Directories cannot be opened as files */
	if (inode->i_mode & S_IFDIR) {
		kfree(inode);
		return -1;
	}

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
 * @fd: File descriptor (returned by vfs_open)
 * @buf: User buffer to store data
 * @count: Number of bytes to read
 *
 * Return: Number of bytes read, -1 on error, or 0 if EOF.
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
 * @fd: File descriptor (returned by vfs_open)
 * @buf: User buffer containing data
 * @count: Number of bytes to write
 *
 * Return: Number of bytes written, -1 on error.
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
 * @fd: File descriptor (returned by vfs_open)
 *
 * Return: 0 on success, -1 on error.
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

/**
 * vfs_chdir() - Validate a path for use as working directory
 * @path: Target path — only "/" and devfs paths ("/dev") are allowed.
 *
 * FAT32 subdirectories (/bin, /sbin, etc.) are rejected.
 * Return: 0 if allowed, -1 otherwise.
 */
int vfs_chdir(const char *path)
{
	const char *p = path;
	while (*p == '/')
		p++;

	/* Root "/" is always valid */
	if (*p == '\0')
		return 0;

	/* /dev itself is valid; /dev/<name> is not a directory */
	if (devfs_sb && is_dev_prefix(p)) {
		const char *sub = p + 3;
		while (*sub == '/')
			sub++;
		return (*sub == '\0') ? 0 : -1;
	}

	return -1;
}

/**
 * vfs_ioctl() - Device control via file descriptor
 * @fd:  File descriptor (returned by vfs_open)
 * @cmd: ioctl command (encoded with _IOC macros)
 * @arg: Command argument (pointer or value)
 *
 * Dispatches to the driver's ioctl file_operation.
 * Return: driver-defined value, -1 if not supported or bad fd.
 */
int vfs_ioctl(int fd, unsigned int cmd, unsigned long arg)
{
	if (fd < 0 || fd >= MAX_FDS || !fd_table[fd])
		return -1;
	struct file *f = fd_table[fd];
	if (f->f_op && f->f_op->ioctl)
		return f->f_op->ioctl(f, cmd, arg);
	return -1;
}

/**
 * vfs_listdir() - List directory contents
 * @path: Path to the directory
 * @buf: Buffer to store file_entry array
 * @max: Maximum number of entries to return
 *
 * Return: Number of entries filled in @buf, -1 on error.
 */
int vfs_listdir(const char *path, struct file_entry *buf, int max)
{
	/* Skip leading slashes for prefix check */
	const char *p = path;
	while (*p == '/')
		p++;

	/* /dev/ listing goes to devfs */
	if (devfs_sb && is_dev_prefix(p)) {
		struct inode *dir = devfs_sb->s_root->d_inode;
		return devfs_sb->s_op->readdir(dir, buf, max);
	}

	if (!root_sb || !root_sb->s_op || !root_sb->s_op->readdir)
		return -1;

	struct inode *dir = walk_path(path);
	if (!dir || !(dir->i_mode & S_IFDIR))
		return -1;

	int n = root_sb->s_op->readdir(dir, buf, max);

	/* Inject synthetic "DEV/" entry when listing root and devfs is mounted */
	if (devfs_sb && *p == '\0' && n >= 0 && n < max) {
		buf[n].name[0] = 'D';
		buf[n].name[1] = 'E';
		buf[n].name[2] = 'V';
		buf[n].name[3] = '/';
		buf[n].name[4] = '\0';
		buf[n].size = 0;
		n++;
	}

	return n;
}
