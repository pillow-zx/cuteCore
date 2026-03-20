/**
 * @file balloc.c
 * @brief Ext2 文件系统块分配与释放
 *
 * 本文件实现了 Ext2 文件系统的块分配器，负责管理磁盘块的分配和释放操作。
 * 块分配器通过操作块位图(bitmap)来跟踪每个块组中空闲块的使用状态。
 */

#include "ext2/ext2.h"
#include "fs.h"

/**
 * @brief 分配一个新的块
 *
 * 遍历所有块组，在块位图中查找第一个空闲块并标记为已使用。
 * 分配成功后会更新块组描述符和超级块中的空闲块计数。
 *
 * @return 成功返回分配的物理块号；失败返回 0
 *
 * @note 块号从 s_first_data_block 开始，即第一个数据块
 * @note 分配策略：从第一个块组开始顺序查找，找到第一个空闲块即返回
 */
u32 ext2_alloc_block(void)
{
	const auto sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	// 遍历所有块组
	for (u32 g = 0; g < sbi->group_count; g++) {
		// 获得对应块组的块组描述符表
		struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

		// 检查当前块组是否存在空闲块
		if (gdesc->bg_free_blocks_count == 0)
			continue;

		// 读取块组描述符表
		struct blk_cache *bmap = bread(gdesc->bg_block_bitmap);
		if (bmap == nullptr)
			return 0;

		u32 base = raw_sb->s_first_data_block +
			   g * raw_sb->s_blocks_per_group;
		u32 blocks_in_group = raw_sb->s_blocks_per_group;
		if (base + blocks_in_group > raw_sb->s_blocks_count)
			blocks_in_group = raw_sb->s_blocks_count - base;

		u32 bit = 0;
		bool found = false;
		u32 nbytes = (blocks_in_group + 7) / 8;
		for (u32 byte = 0; byte < nbytes && !found; byte++) {
			const u8 b = (u8)bmap->data[byte];
			if (b == 0xFF)
				continue;
			for (u32 k = 0; k < 8; k++) {
				if (b & (1u << k))
					continue;
				bit = byte * 8 + k;
				if (bit >= blocks_in_group)
					break;
				bmap->data[byte] = (char)((u8)bmap->data[byte] |
							  (u8)(1u << k));
				found = true;
				break;
			}
		}

		if (!found) {
			brelse(bmap);
			continue;
		}

		bwrite(bmap);
		brelse(bmap);

		gdesc->bg_free_blocks_count--;

		u32 gdesc_per_blk = fs.block_size / sizeof(struct ext2_bg_desc);
		const u32 gdt_blk =
			raw_sb->s_first_data_block + 1 + g / gdesc_per_blk;
		const u32 gdt_off =
			g % gdesc_per_blk * sizeof(struct ext2_bg_desc);
		struct blk_cache *gdt_cache = bread(gdt_blk);
		if (gdt_cache != nullptr) {
			memcpy(gdt_cache->data + gdt_off, gdesc,
			       sizeof(struct ext2_bg_desc));
			bwrite(gdt_cache);
			brelse(gdt_cache);
		}

		raw_sb->s_free_blocks_count--;
		bwrite(sbi->sb_buf);

		return base + bit;
	}

	return 0;
}

/**
 * @brief 释放一个已分配的块
 *
 * 根据块号计算其所在的块组和位图位置，清除位图中对应的占用标志。
 * 释放成功后会更新块组描述符和超级块中的空闲块计数。
 *
 * @param blk_no 要释放的物理块号
 *
 * @note 无效块号（0 或小于 s_first_data_block）将被忽略
 * @note 超出文件系统范围的块号将被忽略
 */
void ext2_free_block(const u32 blk_no)
{
	const auto sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	if (blk_no == 0)
		return;
	if (blk_no < raw_sb->s_first_data_block)
		return;

	u32 rel = blk_no - raw_sb->s_first_data_block;
	u32 g = rel / raw_sb->s_blocks_per_group;
	u32 bit = rel % raw_sb->s_blocks_per_group;

	if (g >= sbi->group_count)
		return;

	struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

	struct blk_cache *bmap = bread(gdesc->bg_block_bitmap);
	if (bmap == nullptr)
		return;
	const u32 byte = bit / 8;
	const u32 k = bit % 8;
	bmap->data[byte] = (char)((u8)bmap->data[byte] & (u8) ~(1u << k));
	bwrite(bmap);
	brelse(bmap);

	gdesc->bg_free_blocks_count++;
	u32 gdesc_per_blk = fs.block_size / sizeof(struct ext2_bg_desc);
	const u32 gdt_blk = raw_sb->s_first_data_block + 1 + g / gdesc_per_blk;
	const u32 gdt_off = g % gdesc_per_blk * sizeof(struct ext2_bg_desc);
	struct blk_cache *gdt_cache = bread(gdt_blk);
	if (gdt_cache != nullptr) {
		memcpy(gdt_cache->data + gdt_off, gdesc,
		       sizeof(struct ext2_bg_desc));
		bwrite(gdt_cache);
		brelse(gdt_cache);
	}

	raw_sb->s_free_blocks_count++;
	bwrite(sbi->sb_buf);
}
