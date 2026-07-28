#ifndef _NOTHAN_FS_H
#define _NOTHAN_FS_H

#include <nothan/types.h>
#include <nothan/genhd.h>

#define MAX_FILENAME 256
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040

#define S_IFMT   0xF000
#define S_IFDIR  0x4000
#define S_IFREG  0x8000
#define S_IFCHR  0x2000

#define FILE_NAME_LEN 32

struct inode;
struct file;
struct super_block;
struct dentry;
struct file_entry;

struct file_operations {
	int (*read)(struct file *file, char *buf, size_t count);
	int (*write)(struct file *file, const char *buf, size_t count);
	int (*open)(struct inode *inode, struct file *file);
	int (*release)(struct inode *inode, struct file *file);
	int (*ioctl)(struct file *file, unsigned int cmd, unsigned long arg);
};

struct super_operations {
	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *inode);
	int (*read_inode)(struct inode *inode);
	struct inode *(*lookup_root)(struct super_block *sb, const char *name);
	struct inode *(*dirlookup)(struct inode *dir, const char *name);
	struct inode *(*create)(struct super_block *sb, struct inode *dir,
				const char *name);
	int (*readdir)(struct inode *dir, struct file_entry *buf, int max);
};

struct super_block {
	struct gendisk *s_bdev;   /* backing block device (gendisk) */
	const struct super_operations *s_op;
	struct dentry *s_root;
	void *s_fs_info;
	uint32_t s_blocksize;
};

struct inode {
	uint32_t i_ino;
	uint32_t i_size;
	uint32_t i_mode;
	uint32_t i_blocks;
	struct super_block *i_sb;
	const struct file_operations *i_fop;
	void *i_private;
};

struct dentry {
	char d_name[MAX_FILENAME];
	struct inode *d_inode;
	struct super_block *d_sb;
	struct dentry *d_parent;
};

struct file {
	struct dentry *f_dentry;
	struct inode *f_inode;
	const struct file_operations *f_op;
	uint32_t f_pos;
	uint32_t f_flags;
	void *private_data;
};

struct file_entry {
	char name[FILE_NAME_LEN];
	unsigned long size;
};

#define MAX_FDS		16

/**
 * struct files_struct - one process's open files
 * @fd: descriptor slots; NULL = free. fd 0/1/2 are reserved for the UART
 *      console and are never handed out (see FD_FIRST in vfs.c).
 *
 * Per process, not global.  A global table meant any process could reach any
 * other process's open file by guessing a small integer - and, worse, that two
 * tasks could legitimately hold the same fd number, which turns the
 * "check non-NULL then use" in vfs_read/write/ioctl into a use-after-free the
 * moment one of them closes.
 *
 * No lock here, and the reason is a decision rather than an oversight: a
 * process is single-threaded (C6, see Documentation/process-mm-design.md), so
 * exactly one task ever reaches a given table.  The day threads exist, this
 * struct is where their lock goes, and struct file will need a refcount so a
 * close on one thread cannot free a file another is mid-read on.
 */
struct files_struct {
	struct file *fd[MAX_FDS];
};

struct files_struct *files_alloc(void);		/* empty table, or NULL */
void		     files_free(struct files_struct *files);	/* closes all */

/* PID 0's table. Static so it exists before the allocator has users; see vfs.c. */
extern struct files_struct init_files;

/* VFS API */
int vfs_mount(const char *dev_name, const char *fs_type);
int vfs_open(const char *pathname, int flags);
int vfs_read(int fd, char *buf, size_t count);
int vfs_write(int fd, const char *buf, size_t count);
int vfs_close(int fd);
int vfs_ioctl(int fd, unsigned int cmd, unsigned long arg);
int vfs_chdir(const char *path);
int vfs_listdir(const char *path, struct file_entry *buf, int max);

#endif /* _NOTHAN_FS_H */
