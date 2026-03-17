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

/**
 * @brief VFS inode 类型（复用 POSIX 文件类型位）。
 *
 * 该枚举的取值与 `mode & S_IFMT` 的类型位一致，便于与 `S_ISREG/S_ISDIR`
 * 等宏配合使用。
 */
enum inode_t : u32 {
	INODE_FILE = S_IFREG,
	INODE_DIR = S_IFDIR,
	INODE_SYNLINK = S_IFLNK,
};

/**
 * @brief VFS inode（文件系统对象的元数据抽象）。
 *
 * `inode` 描述“文件/目录本身”，可被多个 `struct file`（打开实例）共享。
 * 具体文件系统可以通过 `fs_private` 绑定其私有表示（如 ext2 inode 缓存）。
 */
struct inode {
	u32 ino; /**< inode 号（文件系统内唯一标识）。 */
	enum inode_t type; /**< 对象类型（文件/目录/符号链接）。 */
	u32 mode; /**< 权限与类型位（类似 POSIX `st_mode`）。 */
	u32 uid; /**< 属主用户 ID。 */
	u32 gid; /**< 属主组 ID。 */
	u32 size; /**< 文件大小（字节）。 */
	u32 nlinks; /**< 硬链接计数（目录项引用数）。 */

	i32 ref; /**< 内存引用计数（持有该 inode 的引用数）。 */
	bool dirty; /**< 元数据被修改，需要回写到底层文件系统。 */
	bool valid; /**< 元数据是否已从底层文件系统加载完成。 */

	struct sleeplock
		lock; /**< 保护该 inode 的睡眠锁（可能在 I/O 路径持有）。 */

	struct vfs *fs; /**< 所属 VFS 实例（挂载的文件系统）。 */
	struct inode_ops *iops; /**< inode 操作表（由具体文件系统提供）。 */
	struct file_ops *fops; /**< 默认 file 操作表（由具体文件系统提供）。 */

	void *fs_private; /**< 具体文件系统的私有数据指针。 */

	struct list_head
		cache_link; /**< inode 缓存链表节点（挂到 `vfs::inode_cache`）。 */

	u32 dev; /**< 底层设备号（区分不同设备上的 inode）。 */
};

/**
 * @brief 打开文件对象（一次 open 的运行时状态）。
 *
 * `file` 关联到一个 `inode`，保存本次打开的 flags 与当前偏移；同一 inode
 * 可以对应多个 `file` 实例（多次打开或 dup）。
 */
struct file {
	i32 ref; /**< 打开文件对象引用计数。 */
	i32 flags; /**< 打开标志（如 `O_RDONLY/O_WRONLY/O_RDWR/...`）。 */
	off_t offset; /**< 当前读写偏移（每个打开实例独立）。 */

	struct inode *inode; /**< 关联的 inode。 */
	struct file_ops *ops; /**< 文件操作表（read/write/release 等）。 */
};

/**
 * @brief VFS 实例（一次挂载的文件系统）。
 *
 * 保存该文件系统的全局参数、根目录 inode、操作表以及 inode 缓存等信息。
 */
struct vfs {
	u32 block_size; /**< 文件系统块大小（字节）。 */
	u32 blocks_count; /**< 文件系统总块数。 */

	struct inode *root; /**< 根目录 inode（该挂载点的根）。 */
	struct inode_ops *inode_ops; /**< 默认 inode 操作表。 */
	struct file_ops *file_ops; /**< 默认 file 操作表。 */

	void *fs_private; /**< 具体文件系统实例私有数据（如 superblock）。 */

	struct list_head inode_cache; /**< inode 缓存链表头。 */
	u32 icache_size; /**< inode 缓存大小/计数（用于回收/限制）。 */

	u32 dev; /**< 底层设备号（该挂载实例对应的设备）。 */
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
