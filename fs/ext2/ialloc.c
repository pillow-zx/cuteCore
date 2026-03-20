/**
 * @file ialloc.c
 * @brief Ext2 文件系统 inode 分配与释放
 *
 * 本文件实现了 Ext2 文件系统的 inode 分配器，负责管理 inode 的分配和释放操作。
 * inode 分配器通过操作 inode 位图来跟踪每个块组中空闲 inode 的使用状态。
 */

#include "ext2/ext2.h"
#include "fs.h"

/**
 * @brief 分配一个新的 inode
 *
 * 遍历所有块组，在 inode 位图中查找第一个空闲 inode 并标记为已使用。
 * 分配成功后会更新块组描述符和超级块中的空闲 inode 计数。
 *
 * @return 成功返回分配的 inode 号（从 1 开始）；失败返回 0
 *
 * @note inode 号从 1 开始，返回值需直接用于 ext2_iget 等函数
 * @note 分配策略：从第一个块组开始顺序查找，找到第一个空闲 inode 即返回
 */
u32 ext2_alloc_inode(void)
{
	const auto sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	for (u32 g = 0; g < sbi->group_count; g++) {
		struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

		if (gdesc->bg_free_inodes_count == 0)
			continue;

		struct blk_cache *bmap = bread(gdesc->bg_inode_bitmap);
		if (bmap == nullptr)
			return 0;

		u32 inodes_in_group = sbi->inode_per_group;
		const u32 base_ino = g * sbi->inode_per_group;
		if (base_ino + inodes_in_group > raw_sb->s_inode_count)
			inodes_in_group = raw_sb->s_inode_count - base_ino;

		u32 bit = 0;
		bool found = false;
		u32 nbytes = (inodes_in_group + 7) / 8;
		for (u32 byte = 0; byte < nbytes && !found; byte++) {
			const u8 b = (u8)bmap->data[byte];
			if (b == 0xFF)
				continue;
			for (u32 k = 0; k < 8; k++) {
				if (b & 1u << k)
					continue;
				bit = byte * 8 + k;
				if (bit >= inodes_in_group)
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

		gdesc->bg_free_inodes_count--;
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

		raw_sb->s_free_inodes_count--;
		bwrite(sbi->sb_buf);

		return base_ino + bit + 1;
	}

	return 0;
}

/**
 * @brief 释放一个已分配的 inode
 *
 * 根据 inode 号计算其所在的块组和位图位置，清除位图中对应的占用标志。
 * 释放成功后会更新块组描述符和超级块中的空闲 inode 计数。
 *
 * @param ino 要释放的 inode 号
 *
 * @note 无效 inode 号（0 或超出范围）将被忽略
 */
void ext2_free_inode(const u32 ino)
{
	const auto sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	if (ino == 0)
		return;

	const u32 idx = ino - 1;
	const u32 g = idx / sbi->inode_per_group;
	const u32 bit = idx % sbi->inode_per_group;

	if (g >= sbi->group_count)
		return;

	struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

	struct blk_cache *bmap = bread(gdesc->bg_inode_bitmap);
	if (bmap == nullptr)
		return;
	const u32 byte = bit / 8;
	const u32 k = bit % 8;
	bmap->data[byte] = (char)((u8)bmap->data[byte] & (u8) ~(1u << k));
	bwrite(bmap);
	brelse(bmap);

	gdesc->bg_free_inodes_count++;
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

	raw_sb->s_free_inodes_count++;
	bwrite(sbi->sb_buf);
}
