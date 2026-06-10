#ifndef _NOTHAN_FS_H
#define _NOTHAN_FS_H

#include <nothan/types.h>
#include <nothan/block.h>

#define MAX_FILENAME 256
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002

struct inode;
struct file;
struct super_block;
struct dentry;

struct file_operations {
	int (*read)(struct file *file, char *buf, size_t count);
	int (*write)(struct file *file, const char *buf, size_t count);
	int (*open)(struct inode *inode, struct file *file);
	int (*release)(struct inode *inode, struct file *file);
};

struct super_operations {
	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *inode);
	int (*read_inode)(struct inode *inode);
	struct inode *(*lookup_root)(struct super_block *sb, const char *name);
};

struct super_block {
	struct block_device *s_bdev;
	const struct super_operations *s_op;
	struct dentry *s_root;
	void *s_fs_info; /* FS specific data */
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

/* VFS API */
int vfs_mount(const char *dev_name, const char *fs_type);
int vfs_open(const char *pathname, int flags);
int vfs_read(int fd, char *buf, size_t count);
int vfs_write(int fd, const char *buf, size_t count);
int vfs_close(int fd);

#endif /* _NOTHAN_FS_H */
