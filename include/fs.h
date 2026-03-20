/**
 * @file fs.h
 * @brief 虚拟文件系统（VFS）接口定义
 *
 * 本文件定义了 VFS 层的核心数据结构和接口，包括：
 * - inode 与 file 结构体
 * - 文件系统操作表（inode_ops / file_ops）
 * - 块缓存结构
 * - 路径解析与 inode 管理接口
 *
 * VFS 层提供统一的文件系统抽象，使上层代码无需关心底层文件系统实现。
 */

#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#include "lock.h"
#include "config.h"
#include "list.h"

/* ============================================================================
 * POSIX File Type Masks
 * ============================================================================ */

/** @brief 文件类型掩码（用于提取文件类型位） */
#define S_IFMT 0170000u

/** @brief 普通文件类型位 */
#define S_IFREG 0100000u

/** @brief 目录类型位 */
#define S_IFDIR 0040000u

/** @brief 符号链接类型位 */
#define S_IFLNK 0120000u

/* ============================================================================
 * POSIX Permission Bits
 * ============================================================================ */

/** @brief 用户读权限 */
#define S_IRUSR 0400u

/** @brief 用户写权限 */
#define S_IWUSR 0200u

/** @brief 用户执行权限 */
#define S_IXUSR 0100u

/* ============================================================================
 * File Type Test Macros
 * ============================================================================ */

/** @brief 判断 mode 是否为普通文件 */
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

/** @brief 判断 mode 是否为目录 */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/** @brief 判断 mode 是否为符号链接 */
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)

/* ============================================================================
 * File Open Flags
 * ============================================================================ */

/** @brief 只读打开 */
#define O_RDONLY 0

/** @brief 只写打开 */
#define O_WRONLY 1

/** @brief 读写打开 */
#define O_RDWR 2

/** @brief 若文件不存在则创建 */
#define O_CREAT (1 << 6)

/** @brief 与 O_CREAT 一起使用，若文件已存在则失败 */
#define O_EXCL (1 << 7)

/** @brief 打开时截断文件长度为 0 */
#define O_TRUNC (1 << 9)

/** @brief 以追加模式打开（写入时定位到文件末尾） */
#define O_APPEND (1 << 10)

/* ============================================================================
 * File Access Test Macros
 * ============================================================================ */

/** @brief 测试文件是否可读（flags 低 2 位不等于 O_WRONLY） */
#define FILE_READABLE(f) (((f)->flags & 3) != O_WRONLY)

/** @brief 测试文件是否可写（flags 低 2 位不等于 O_RDONLY） */
#define FILE_WRITABLE(f) (((f)->flags & 3) != O_RDONLY)

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/** @brief 文件偏移量类型 */
typedef uint32_t off_t;

/* ============================================================================
 * Core Data Structures
 * ============================================================================ */

/**
 * @brief VFS inode（文件系统对象的元数据抽象）。
 *
 * `inode` 描述"文件/目录本身"，可被多个 `struct file`（打开实例）共享。
 * 具体文件系统可以通过 `fs_private` 绑定其私有表示（如 ext2 inode 缓存）。
 */
struct inode {
	u32 ino; /**< inode 号（文件系统内唯一标识）。 */
	enum {
		INODE_FILE,
		INODE_DIR,
		INODE_SYNLINK,
		INODE_DEV
	} type; /**< 对象类型（文件/目录/符号链接）。 */
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

	struct list_head f_link; /**< 挂载到全局文件表链表。 */
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

/* ============================================================================
 * Inode Operations Table
 * ============================================================================ */

/**
 * @brief inode 操作表
 *
 * 定义了具体文件系统必须实现的 inode 级别操作。
 * 通过此操作表，VFS 层可以与不同文件系统解耦。
 */
struct inode_ops {
	/**
	 * @brief 从磁盘读取 inode 元数据
	 *
	 * @param inode 目标 inode 结构体指针
	 *
	 * @return 成功返回 0；失败返回负值
	 */
	i32 (*read_inode)(struct inode *inode);

	/**
	 * @brief 将 inode 元数据写入磁盘
	 *
	 * @param inode 目标 inode 结构体指针
	 *
	 * @return 成功返回 0；失败返回负值
	 */
	i32 (*write_inode)(struct inode *inode);

	/**
	 * @brief 销毁 inode 结构体（释放内存）
	 *
	 * @param inode 目标 inode 结构体指针
	 */
	void (*destroy_inode)(struct inode *inode);

	/**
	 * @brief 在目录中查找指定名称的文件
	 *
	 * @param dir     目录 inode
	 * @param name    文件名
	 * @param namelen 文件名长度
	 *
	 * @return 找到返回 inode 指针（引用计数已增）；未找到返回 nullptr
	 */
	struct inode *(*lookup)(const struct inode *dir, const char *name,
				i32 namelen);

	/**
	 * @brief 在目录中创建新文件
	 *
	 * @param dir     父目录 inode
	 * @param name    文件名
	 * @param namelen 文件名长度
	 * @param mode    文件权限
	 *
	 * @return 成功返回新 inode；失败返回 nullptr
	 */
	struct inode *(*create)(struct inode *dir, const char *name,
				i32 namelen, u32 mode);

	/**
	 * @brief 在目录中创建子目录
	 *
	 * @param dir     父目录 inode
	 * @param name    目录名
	 * @param namelen 目录名长度
	 * @param mode    目录权限
	 *
	 * @return 成功返回新目录 inode；失败返回 nullptr
	 */
	struct inode *(*mkdir)(struct inode *dir, const char *name, i32 namelen,
			       u32 mode);

	/**
	 * @brief 从目录中删除目录项
	 *
	 * @param dir     父目录 inode
	 * @param name    文件名
	 * @param namelen 文件名长度
	 *
	 * @return 成功返回 0；失败返回负值
	 */
	i32 (*unlink)(struct inode *dir, const char *name, i32 namelen);

	/**
	 * @brief 释放 inode 占用的所有数据块（截断为 0 字节）
	 *
	 * @param inode 目标 inode
	 */
	void (*truncate)(struct inode *inode);

	/**
	 * @brief 释放 inode 号（回收磁盘 inode 资源）
	 *
	 * @param ino inode 号
	 */
	void (*free_inode)(u32 ino);
};

/* ============================================================================
 * File Operations Table
 * ============================================================================ */

/**
 * @brief 文件操作表
 *
 * 定义了具体文件系统必须实现的文件级别操作。
 * 每次打开文件时，file 结构体通过此表调用底层实现。
 */
struct file_ops {
	/**
	 * @brief 打开文件
	 *
	 * @param inode 文件 inode
	 * @param file  文件结构体
	 *
	 * @return 成功返回 0；失败返回负值
	 */
	i32 (*open)(struct inode *inode, struct file *file);

	/**
	 * @brief 关闭文件
	 *
	 * @param inode 文件 inode
	 * @param file  文件结构体
	 */
	void (*release)(struct inode *inode, struct file *file);

	/**
	 * @brief 读取文件内容
	 *
	 * @param file   文件结构体
	 * @param buf    输出缓冲区
	 * @param len    请求读取的字节数
	 * @param offset 文件偏移量指针（读取后会更新）
	 *
	 * @return 成功返回实际读取的字节数；失败返回负值
	 */
	i32 (*read)(const struct file *file, void *buf, usize len,
		    off_t *offset);

	/**
	 * @brief 写入文件内容
	 *
	 * @param file   文件结构体
	 * @param buf    输入缓冲区
	 * @param len    请求写入的字节数
	 * @param offset 文件偏移量指针（写入后会更新）
	 *
	 * @return 成功返回实际写入的字节数；失败返回负值
	 */
	i32 (*write)(struct file *file, const void *buf, usize len,
		     off_t *offset);
};

/* ============================================================================
 * Block Cache
 * ============================================================================ */

/**
 * @brief 块缓存结构体
 *
 * 缓存单个磁盘块的数据，支持脏标记和引用计数。
 */
struct blk_cache {
	u64 blk_no; /**< 缓存的块号 */
	bool valid; /**< 数据是否有效 */
	bool dirty; /**< 数据是否被修改（需要写回） */
	i32 refcnt; /**< 引用计数 */
	char data[BLKSZ]; /**< 块数据缓冲区 */
};

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/** @brief 全局块缓存数组 */
extern struct blk_cache blk_caches[NBLKNUM];

/** @brief 全局文件系统实例（当前仅支持单个挂载点） */
extern struct vfs fs;

/* ============================================================================
 * Block Cache Functions
 * ============================================================================ */

/**
 * @brief 获取块数据
 *
 * 若块已在缓存中则直接返回；否则从磁盘读取并缓存。
 *
 * @param no 目标块号
 *
 * @return 成功返回块缓存指针；失败触发 panic
 *
 * @note 返回的块引用计数已增，使用完毕应调用 brelse 释放
 */
struct blk_cache *bread(u64 no);

/**
 * @brief 将缓存块标记为脏
 *
 * @param blk 待标记的块缓存指针
 *
 * @note 脏块将在被替换时或 bsync 调用时写回磁盘
 */
void bwrite(struct blk_cache *blk);

/**
 * @brief 释放块缓存引用
 *
 * @param blk 待释放的块缓存指针
 *
 * @note 引用计数降为 0 后该槽位可被重新分配
 */
void brelse(struct blk_cache *blk);

/**
 * @brief 同步所有脏块到磁盘
 *
 * 遍历缓存，将所有脏块写回磁盘并清除脏标志。
 */
void bsync(void);

/* ============================================================================
 * VFS Functions
 * ============================================================================ */

/**
 * @brief 初始化文件系统
 *
 * 初始化全局 VFS 结构体并挂载根文件系统。
 */
void fsinit(void);

/**
 * @brief 释放 inode 引用
 *
 * 减少引用计数，若降为 0 则可能触发删除或写回。
 *
 * @param inode 目标 inode 指针
 */
void iput(struct inode *inode);

/**
 * @brief 解析路径获取目标 inode
 *
 * @param path 绝对路径字符串
 *
 * @return 成功返回 inode 指针（引用计数已增）；失败返回 nullptr
 */
struct inode *namei(const char *path);

/**
 * @brief 解析路径获取父目录 inode
 *
 * @param path 绝对路径字符串
 * @param name 输出参数，存储最后一个路径分量名称
 *
 * @return 成功返回父目录 inode 指针（引用计数已增）；失败返回 nullptr
 */
struct inode *nameiparent(const char *path, char *name);

/* ============================================================================
 * Global File Table
 * ============================================================================ */

/**
 * @brief 全局打开文件表
 *
 * 管理所有打开的 file 结构，支持未来多核扩展（预留锁字段）。
 */
struct ftable {
	struct list_head head; /**< 文件链表头。 */
	usize count; /**< 当前打开文件数。 */
};

/** @brief 全局文件表实例 */
extern struct ftable ftable;

/* ============================================================================
 * File Management Functions
 * ============================================================================ */

/**
 * @brief 初始化全局文件表
 *
 * 在内核启动时调用，初始化文件链表头。
 */
void fileinit(void);

/**
 * @brief 分配一个新的打开文件对象
 *
 * 从 slab 分配器分配 file 结构，初始化字段并加入全局文件表。
 *
 * @return 成功返回 file 指针；失败返回 nullptr
 */
struct file *filealloc(void);

/**
 * @brief 关闭文件并释放引用
 *
 * 减少引用计数，当 ref 降为 0 时：
 * - 调用 inode 的 release 操作
 * - 释放 inode 引用
 * - 从全局文件表移除
 * - 释放 file 结构内存
 *
 * @param f 目标文件对象
 */
void fileclose(struct file *f);

/**
 * @brief 增加文件引用计数
 *
 * 用于 dup、fork 等场景共享同一打开文件对象。
 *
 * @param f 目标文件对象
 *
 * @return 返回传入的文件指针
 */
struct file *filedup(struct file *f);

/**
 * @brief 获取当前打开文件数量
 *
 * @return 当前全局文件表中的文件数
 */
usize filecount(void);

/* ============================================================================
 * Inode I/O Functions
 * ============================================================================ */

/**
 * @brief 从 inode 读取数据
 *
 * 直接从 inode 的指定偏移量读取数据，无需打开文件对象。
 * 用于内核内部访问文件（如加载 ELF 程序）。
 *
 * @param inode  目标 inode
 * @param buf    输出缓冲区
 * @param offset 起始偏移量
 * @param len    请求读取的字节数
 *
 * @return 成功返回实际读取的字节数；失败返回负值
 */
i32 readi(struct inode *inode, void *buf, off_t offset, usize len);

/**
 * @brief 向 inode 写入数据
 *
 * 直接向 inode 的指定偏移量写入数据，无需打开文件对象。
 * 用于内核内部修改文件。
 *
 * @param inode  目标 inode
 * @param buf    输入缓冲区
 * @param offset 起始偏移量
 * @param len    请求写入的字节数
 *
 * @return 成功返回实际写入的字节数；失败返回负值
 */
i32 writei(struct inode *inode, const void *buf, off_t offset, usize len);

/* ============================================================================
 * File I/O Functions
 * ============================================================================ */

/**
 * @brief 从打开的文件读取数据
 *
 * 从文件的当前偏移量读取数据，读取成功后更新偏移量。
 * 仅支持普通文件（INODE_FILE）。
 *
 * @param f   打开的文件对象
 * @param buf 输出缓冲区
 * @param len 请求读取的字节数
 *
 * @return 成功返回实际读取的字节数；失败返回负值
 */
i32 fileread(struct file *f, void *buf, usize len);

/**
 * @brief 向打开的文件写入数据
 *
 * 从文件的当前偏移量写入数据，写入成功后更新偏移量。
 * 仅支持普通文件（INODE_FILE）。
 *
 * @param f   打开的文件对象
 * @param buf 输入缓冲区
 * @param len 请求写入的字节数
 *
 * @return 成功返回实际写入的字节数；失败返回负值
 */
i32 filewrite(struct file *f, const void *buf, usize len);

#endif
