/**
 * @file super.c
 * @brief Ext2 文件系统超级块与初始化
 *
 * 本文件实现了 Ext2 文件系统的初始化流程，包括超级块读取验证、
 * 块组描述符表加载、文件系统布局计算等核心功能。
 */

#include "ext2/ext2.h"
#include "fs.h"
#include "utils.h"

/** @brief 计算块组描述符表总大小 */
#define EXT2_GDESC_SIZE(c) ((c) * sizeof(struct ext2_bg_desc))

/**
 * @brief Ext2 文件系统初始化
 *
 * 完成以下初始化步骤：
 * 1. 读取并验证超级块（校验 magic number）
 * 2. 计算文件系统布局参数（块大小、块组数量、inode 尺寸）
 * 3. 加载块组描述符表（GDT）
 * 4. 挂载根目录 inode
 *
 * @return 成功返回 0；失败返回负值错误码
 *         -1: 内存分配失败或 I/O 错误
 *         -2: 超级块 magic number 校验失败
 *
 * @note 初始化成功后，全局 fs 结构体将包含文件系统实例信息
 */
i32 ext2_fs_init(void)
{
	// 1. 读取并验证super block

	// 从磁盘读取super block
	struct blk_cache *cache = bread(EXT2_SUPER_BLKNUM);
	if (cache == nullptr)
		return -1;

	// 构造super block struct
	const auto sp =
		(struct ext2_super_block *)(cache->data + EXT2_SUPER_OFFSET);

	// 校验 maigc number
	if (sp->s_magic != EXT2_SUPER_MAGIC) {
		brelse(cache);
		return -2;
	}

	// 创建ext2_info用户保存文件系统运行实例的私有运行时数据
	struct ext2_sb_info *ext2_info = kmalloc(sizeof(struct ext2_sb_info));
	if (ext2_info == nullptr) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}
	ext2_info->sb_buf = cache;
	ext2_info->raw_sb =
		(struct ext2_super_block *)(cache->data + EXT2_SUPER_OFFSET);

	// 2. 计算文件系统布局

	// 计算ext2块大小
	// ext2 块大小是动态的，计算公式为block_size = 1024 * 2 ^ s_log_block_size
	if (sp->s_log_block_size > EXT2_MAX_LOG_BLOCK_SIZE) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}
	const u32 block_size = 1024U << sp->s_log_block_size;

	// 计算ext2块组数量
	// 计算公式为group_count / blocks_per_group
	if (sp->s_blocks_per_group == 0 || sp->s_inodes_per_group == 0) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}
	ext2_info->group_count = (sp->s_blocks_count - sp->s_first_data_block +
				  sp->s_blocks_per_group - 1) /
				 sp->s_blocks_per_group;
	if (ext2_info->group_count == 0) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	// 获得每个块组内inode数量
	ext2_info->inode_per_group = sp->s_inodes_per_group;

	// 计算inode尺寸
	ext2_info->inode_size = (sp->s_rev_level >= EXT2_DYNAMIC_REV) ?
					(u32)sp->s_inode_size :
					EXT2_GOOD_OLD_INODE_SIZE;

	// 3. 加载块组描述符表(GDT)
	const usize gdesc_size = EXT2_GDESC_SIZE(ext2_info->group_count);
	ext2_info->gb_table = kmalloc(gdesc_size);
	if (ext2_info->gb_table == nullptr) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	// - 计算GDT总字节大小
	// - 从s_first_data_block + 1开始逐块读取
	// - 使用memcpy将磁盘块中的数据拼接到内存缓冲区ext2_info->gb_table
	{
		usize copied = 0;
		u64 gdt_blk = sp->s_first_data_block + 1;

		while (copied < gdesc_size) {
			struct blk_cache *c2 = bread(gdt_blk);
			if (c2 == nullptr) {
				kmfree(ext2_info->gb_table);
				brelse(cache);
				kmfree(ext2_info);
				return -1;
			}
			usize chunk = gdesc_size - copied;
			if (chunk > block_size)
				chunk = block_size;
			memcpy((char *)ext2_info->gb_table + copied, c2->data,
			       chunk);
			brelse(c2);
			copied += chunk;
			gdt_blk++;
		}
	}

	// 4. 挂载根目录

	// 初始化剩余内容，注册操作端口
	fs.block_size = block_size;
	fs.blocks_count = sp->s_blocks_count;
	fs.inode_ops = &ext2_inode_ops;
	fs.file_ops = &ext2_file_ops;
	fs.fs_private = ext2_info;
	list_init(&fs.inode_cache);
	fs.icache_size = 0;
	fs.dev = 0;

	// 读取并挂载根节点
	fs.root = ext2_iget(EXT2_ROOT_INO);
	if (fs.root == nullptr) {
		kmfree(ext2_info->gb_table);
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	return 0;
}
