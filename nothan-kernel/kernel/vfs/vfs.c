/*
 * kernel/vfs/vfs.c - Virtual File System core
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */
#include <nothan/fs.h>
#include <nothan/devfs.h>
#include <nothan/genhd.h>
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
	/* devfs has no block device */
	if (fs_type[0] == 'd' && fs_type[1] == 'e' && fs_type[2] == 'v')
		return devfs_mount();

	struct gendisk *disk = gendisk_lookup_by_name(dev_name);
	if (!disk) {
		printk("[VFS] Cannot find disk '%s'\n", dev_name);
		return -1;
	}

	printk("[VFS] Mounting %s on %s\n", fs_type, dev_name);

	struct super_block *sb = (struct super_block *)kmalloc(sizeof(*sb), GFP_KERNEL);
	if (!sb)
		return -1;

	sb->s_bdev = disk;
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

/*
 * vfs_free_inode() - Release an inode via its filesystem's destroy_inode,
 * which knows whether i_private is owned (FAT) or borrowed (devfs cdev).
 * Falls back to a plain kfree if the FS provides no hook.
 */
static void vfs_free_inode(struct inode *inode)
{
	if (!inode)
		return;
	if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->destroy_inode)
		inode->i_sb->s_op->destroy_inode(inode);
	else
		kfree(inode);
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
 * vfs_create_path() - Create a missing file for O_CREAT
 * @pathname: Path of the file to create
 *
 * Splits @pathname into a parent directory and a final component, walks
 * to the parent, and asks the filesystem to create the entry. Only the
 * final component is created — the parent directory must already exist.
 * /dev paths are not creatable.
 *
 * Return: Inode for the new file, or NULL on error.
 */
static struct inode *vfs_create_path(const char *pathname)
{
	if (!root_sb || !root_sb->s_op || !root_sb->s_op->create)
		return NULL;

	while (*pathname == '/')
		pathname++;

	if (devfs_sb && is_dev_prefix(pathname))
		return NULL;

	/* Locate the final path component. */
	const char *last_slash = NULL;
	for (const char *p = pathname; *p; p++)
		if (*p == '/')
			last_slash = p;

	struct inode *parent;
	const char *name;
	int free_parent = 0;

	if (!last_slash) {
		/* File lives directly in the root directory. */
		if (!root_sb->s_root || !root_sb->s_root->d_inode)
			return NULL;
		parent = root_sb->s_root->d_inode;
		name = pathname;
	} else {
		char parent_path[256];
		int len = last_slash - pathname;
		if (len > 255)
			return NULL;
		for (int i = 0; i < len; i++)
			parent_path[i] = pathname[i];
		parent_path[len] = '\0';
		parent = walk_path(parent_path);
		if (!parent || !(parent->i_mode & S_IFDIR)) {
			if (parent)
				vfs_free_inode(parent);
			return NULL;
		}
		free_parent = 1;
		name = last_slash + 1;
	}

	if (*name == '\0') {
		if (free_parent)
			vfs_free_inode(parent);
		return NULL;
	}

	struct inode *inode = root_sb->s_op->create(root_sb, parent, name);

	if (free_parent)
		vfs_free_inode(parent);
	return inode;
}

/**
 * vfs_open() - Open a file
 * @pathname: Path to the file (e.g., "SHELL.BIN", "/bin/hello")
 * @flags: Open flags (O_RDONLY, O_WRONLY, etc.)
 *
 * Return: File descriptor (fd >= 0) on success, -1 on error.
 */
/* Placeholder marking a slot as "reserved while we build the file". Any
 * non-NULL value works — vfs_ioctl/read/write/close re-read fd_table[]
 * and would only follow it as a real pointer; the race is purely about
 * preventing two concurrent vfs_open calls from picking the same fd. */
#define FD_RESERVED  ((struct file *)(uintptr_t)0x1)

/* fd 0/1/2 are stdin/stdout/stderr — the UART, special-cased by NUMBER in
 * sys_read (fd 0) and sys_writefile (fd 1). Never hand those numbers to a real
 * file: if we did, a write to that file would silently go to the UART instead
 * (it does, in the monkey build where /dev/input0 is not opened, so fd 1 is
 * free and a persistence save lands on it). Allocate file fds from 3 up. */
#define FD_FIRST  3

int vfs_open(const char *pathname, int flags)
{
	/*
	 * Atomically reserve a free fd slot before doing the slow work
	 * (walk_path, kmalloc, fops->open) so a preempting vfs_open in
	 * another task cannot pick the same fd.
	 */
	unsigned long cpsr;
	int fd = -1;
	__asm__ __volatile__ ("mrs %0, cpsr\n\tcpsid i" : "=r"(cpsr) : : "memory");
	for (int i = FD_FIRST; i < MAX_FDS; i++) {
		if (!fd_table[i]) {
			fd_table[i] = FD_RESERVED;
			fd = i;
			break;
		}
	}
	if (!(cpsr & (1u << 7)))
		__asm__ __volatile__ ("cpsie i" : : : "memory");

	if (fd < 0) {
		printk("[VFS] Out of file descriptors\n");
		return -1;
	}

	struct inode *inode = walk_path(pathname);
	if (!inode && (flags & O_CREAT))
		inode = vfs_create_path(pathname);
	if (!inode) {
		fd_table[fd] = NULL;
		return -1;
	}

	/* Directories cannot be opened as files */
	if (inode->i_mode & S_IFDIR) {
		vfs_free_inode(inode);
		fd_table[fd] = NULL;
		return -1;
	}

	struct file *f = (struct file *)kmalloc(sizeof(*f), GFP_KERNEL);
	if (!f) {
		vfs_free_inode(inode);
		fd_table[fd] = NULL;
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
			vfs_free_inode(inode);
			fd_table[fd] = NULL;
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
	vfs_free_inode(f->f_inode);
	kfree(f);
	fd_table[fd] = NULL;
	return 0;
}

/**
 * vfs_chdir() - Validate a path for use as working directory
 * @path: Target path — only "/" and devfs paths ("/dev") are allowed.
 *
 * FAT32 subdirectories are rejected — only "/" and "/dev" are valid.
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
