/**
 * @file ext2_def.h
 * @brief Ext2 文件系统数据结构定义
 *
 * 本头文件定义了 Ext2 文件系统的核心数据结构，包括：
 * - 超级块（super block）
 * - 块组描述符（block group descriptor）
 * - inode 结构
 * - 目录项（directory entry）
 * - 运行时信息结构
 */

#ifndef EXT2_DEF_H
#define EXT2_DEF_H

#include "fs.h"
#include "utils.h"

/** @defgroup ext2_constants Ext2 常量定义
 * @{
 */

/** @brief 超级块所在块号 */
#define EXT2_SUPER_BLKNUM 0

/** @brief 超级块在块内的字节偏移（超级块始终位于字节 1024 处） */
#define EXT2_SUPER_OFFSET 1024

/** @brief 块组描述符表的字节偏移 */
#define EXT2_GDESC_OFFSET 2048

/** @brief Ext2 魔数（0xEF53） */
#define EXT2_SUPER_MAGIC 0xEF53

/** @brief 传统 inode 尺寸（128 字节） */
#define EXT2_GOOD_OLD_INODE_SIZE 128

/** @brief 根目录 inode 号 */
#define EXT2_ROOT_INO 2

/** @defgroup ext2_revision Ext2 修订版本
 * @{
 */
/** @brief 原始修订版本 */
#define EXT2_GOOD_OLD_REV 0
/** @brief 动态修订版本（支持可变 inode 尺寸等特性） */
#define EXT2_DYNAMIC_REV 1
/** @} */

/** @brief 最大块大小对数值（2 表示 4096 字节） */
#define EXT2_MAX_LOG_BLOCK_SIZE 2

/** @defgroup ext2_block_indices 块索引常量
 * @{
 */
/** @brief 直接块数量 */
#define EXT2_NDIR_BLOCKS 12
/** @brief 一级间接块索引 */
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
/** @brief 二级间接块索引 */
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
/** @brief 三级间接块索引 */
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
/** @brief 总块指针数量 */
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)
/** @} */

/** @} */ /* end of ext2_constants */

/**
 * @brief Ext2 块组描述符
 *
 * 每个块组对应一个描述符，记录该块组的元信息。
 * 描述符表紧跟在超级块之后。
 */
struct ext2_bg_desc {
	u32 bg_block_bitmap;       /**< 块位图所在块号 */
	u32 bg_inode_bitmap;       /**< inode 位图所在块号 */
	u32 bg_inode_table;        /**< inode 表起始块号 */
	u16 bg_free_blocks_count;  /**< 空闲块数量 */
	u16 bg_free_inodes_count;  /**< 空闲 inode 数量 */
	u16 bg_used_dirs_count;    /**< 已分配目录数量 */
	u16 bg_pad;                /**< 填充 */
	u32 bg_reserved[3];        /**< 保留 */
} __packed;

/**
 * @brief Ext2 inode 结构（磁盘格式）
 *
 * 存储 inode 的元数据和块指针数组。
 */
struct ext2_inode {
	u16 i_mode;           /**< 文件类型和权限 */
	u16 i_uid;            /**< 所有者 UID */
	u32 i_size;           /**< 文件大小（字节） */
	u32 i_atime;          /**< 最后访问时间 */
	u32 i_ctime;          /**< 创建时间 */
	u32 i_mtime;          /**< 最后修改时间 */
	u32 i_dtime;          /**< 删除时间 */
	u16 i_gid;            /**< 所有者 GID */
	u16 i_links_count;    /**< 硬链接计数 */
	u32 i_blocks;         /**< 已分配块数（512 字节为单位） */
	u32 i_flags;          /**< 用户标志 */
	u32 i_reserved1;      /**< 保留 */
	u32 i_block[EXT2_N_BLOCKS]; /**< 块指针数组 */
	u32 i_generation;     /**< 文件版本号 */
	u32 i_file_acl;       /**< 文件 ACL */
	u32 i_dir_acl;        /**< 目录 ACL */
	u32 i_faddr;          /**< 片地址 */

	u8 i_frag;            /**< 片编号 */
	u8 i_fsize;           /**< 片大小 */
	u16 i_pad;            /**< 填充 */
	u16 i_uid_high;       /**< 高 16 位 UID */
	u16 i_gid_high;       /**< 高 16 位 GID */
	u32 i_reserved2;      /**< 保留 */
} __packed;

/**
 * @brief Ext2 超级块结构（磁盘格式）
 *
 * 存储文件系统的全局元信息，位于磁盘字节偏移 1024 处。
 */
struct ext2_super_block {
	u32 s_inode_count;        /**< 总 inode 数量 */
	u32 s_blocks_count;       /**< 总块数量 */
	u32 s_r_blocks_count;     /**< 为超级用户保留的块数 */
	u32 s_free_blocks_count;  /**< 空闲块数量 */
	u32 s_free_inodes_count;  /**< 空闲 inode 数量 */
	u32 s_first_data_block;   /**< 第一个数据块号（0 或 1） */
	u32 s_log_block_size;     /**< 块大小 = 1024 << s_log_block_size */
	u32 s_log_frag_size;      /**< 片大小对数值 */
	u32 s_blocks_per_group;   /**< 每组块数 */
	u32 s_frags_per_group;    /**< 每组片数 */
	u32 s_inodes_per_group;   /**< 每组 inode 数 */
	u32 s_mtime;              /**< 最后挂载时间 */
	u32 s_wtime;              /**< 最后写入时间 */
	u16 s_mnt_count;          /**< 挂载计数 */
	u16 s_max_mnt_count;      /**< 最大挂载计数 */
	u16 s_magic;              /**< 魔数（0xEF53） */
	u16 s_state;              /**< 文件系统状态 */
	u16 s_errors;             /**< 错误处理方式 */
	u16 s_minor_rev_level;    /**< 次版本号 */
	u32 s_lastcheck;          /**< 最后检查时间 */
	u32 s_checkinterval;      /**< 检查间隔 */
	u32 s_creator_os;         /**< 创建者操作系统 */
	u32 s_rev_level;          /**< 修订级别 */
	u16 s_def_resuid;         /**< 默认保留 UID */
	u16 s_def_resgid;         /**< 默认保留 GID */

	u32 s_first_ino;          /**< 第一个非保留 inode 号 */
	u16 s_inode_size;         /**< inode 尺寸 */
	u16 s_block_group_nr;     /**< 块组编号 */
	u32 s_feature_compat;     /**< 兼容特性 */
	u32 s_feature_incompat;   /**< 不兼容特性 */
	u32 s_feature_ro_compat;  /**< 只读兼容特性 */
	u8 s_uuid[16];            /**< 卷 UUID */
	char s_volume_name[16];   /**< 卷名 */
	char s_last_mounted[64];  /**< 最后挂载路径 */
	u32 s_algorithm_usage_bitmap; /**< 算法使用位图 */

	u8 s_prealloc_blocks;     /**< 预分配块数 */
	u8 s_prealloc_dir_blocks; /**< 目录预分配块数 */
	u16 s_padding1;           /**< 填充 */

	u8 s_journal_uuid[16];    /**< 日志 UUID */
	u32 s_journal_inum;       /**< 日志 inode 号 */
	u32 s_journal_dev;        /**< 日志设备号 */
	u32 s_last_orphan;        /**< 孤儿 inode 链表头 */
	u32 s_hash_seed[4];       /**< HTREE 种子 */
	u8 s_def_hash_version;    /**< 默认哈希版本 */
	u8 s_reserved_char_pad;   /**< 保留填充 */
	u16 s_reserved_word_pad;  /**< 保留填充 */
	u32 s_default_mount_opts; /**< 默认挂载选项 */
	u32 s_first_meta_bg;      /**< 第一个元数据块组 */
	u32 s_reserved[190];      /**< 保留 */
} __packed;

/**
 * @brief Ext2 目录项结构（磁盘格式）
 *
 * 目录数据由一系列此结构组成，每个结构表示一个文件/子目录。
 */
struct ext2_dir_entry {
	u32 inode;      /**< inode 号 */
	u16 rec_len;    /**< 本条目长度（字节） */
	u8 name_len;    /**< 文件名长度 */
	u8 file_type;   /**< 文件类型 */
	char name[];    /**< 文件名（柔性数组） */
} __packed;

/**
 * @brief Ext2 超级块运行时信息
 *
 * 内存中维护的文件系统实例私有数据。
 */
struct ext2_sb_info {
	struct ext2_super_block *raw_sb;  /**< 指向磁盘超级块的指针 */
	struct ext2_bg_desc *gb_table;    /**< 块组描述符表 */
	u32 group_count;                  /**< 块组数量 */
	u32 inode_size;                   /**< inode 尺寸 */
	u32 inode_per_group;              /**< 每组 inode 数 */
	struct blk_cache *sb_buf;         /**< 超级块缓存 */
};

/**
 * @brief Ext2 inode 运行时信息
 *
 * 存储在 inode->fs_private 中，保存块指针数组和标志。
 */
struct ext2_inode_info {
	u32 i_block[EXT2_N_BLOCKS];  /**< 块指针数组副本 */
	u32 i_flags;                 /**< inode 标志 */
};

#endif
