#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#include "lock.h"
#include "config.h"
#include "list.h"

#define S_IFMT 0170000u
#define S_IFREG 0100000u
#define S_IFDIR 0040000u
#define S_IFLNK 0120000u

#define S_IRUSR 0400u
#define S_IWUSR 0200u
#define S_IXUSR 0100u

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT (1 << 6)
#define O_EXCL (1 << 7)
#define O_TRUNC (1 << 9)
#define O_APPEND (1 << 10)

#define FILE_READABLE(f) (((f)->flags & 3) != O_WRONLY)
#define FILE_WRITABLE(f) (((f)->flags & 3) != O_RDONLY)

typedef uint32_t off_t;

enum inode_t : u32 {
	INODE_FILE = S_IFREG,
	INODE_DIR = S_IFDIR,
	INODE_SYNLINK = S_IFLNK,
};

struct inode {
	u32 ino;
	enum inode_t type;
	u32 mode;
	u32 uid;
	u32 gid;
	u32 size;
	u32 nlinks;

	i32 ref;
	bool dirty;
	bool valid;

	struct sleeplock lock;

	struct vfs *fs;
	struct inode_ops *iops;
	struct file_ops *fops;

	void *fs_private;

	struct list_head cache_link;

	u32 dev;
};

struct file {
	i32 ref;
	i32 flags;
	off_t offset;

	struct inode *inode;
	struct file_ops *ops;
};

struct vfs {
	u32 block_size;
	u32 blocks_count;

	struct inode *root;
	struct inode_ops *inode_ops;
	struct file_ops *file_ops;

	void *fs_private;

	struct list_head inode_cache;
	u32 icache_size;

	u32 dev;
};

struct inode_ops {
	i32 (*read_inode)(struct inode *inode);
	i32 (*write_inode)(struct inode *inode);
	void (*destroy_inode)(struct inode *inode);

	struct inode *(*lookup)(struct inode *dir, const char *name,
				i32 namelen);
	struct inode *(*create)(struct inode *dir, const char *name,
				i32 namelen, u32 mode);
	struct inode *(*mkdir)(struct inode *dir, const char *name, i32 namelen,
			       u32 mode);
	i32 (*unlink)(struct inode *dir, const char *name, i32 namelen);
};

struct file_ops {
	i32 (*open)(struct inode *inode, struct file *file);
	void (*release)(struct inode *inode, struct file *file);
	i32 (*read)(struct file *file, void *buf, usize len, off_t *offset);
	i32 (*write)(struct file *file, const void *buf, usize len,
		     off_t *offset);
};

struct blk_cache {
	u64 blk_no;
	bool valid;
	bool dirty;
	i32 refcnt;
	char data[BLKSZ];
};

extern struct blk_cache blk_caches[NBLKNUM];
extern struct vfs fs;

struct blk_cache *bread(u64 no);

void bwrite(struct blk_cache *blk);
void brelse(struct blk_cache *blk);
void bsync(void);

i32 ext2_fs_init();
void fsinit(void);

void iput(struct inode *inode);

struct inode *namei(const char *path);

struct inode *nameiparent(const char *path, char *name);

#endif
