#include "ext2.h"
#include "fs.h"

u32 ext2_alloc_inode(void)
{
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	for (u32 g = 0; g < sbi->group_count; g++) {
		struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

		if (gdesc->bg_free_inodes_count == 0)
			continue;

		struct blk_cache *bmap = bread(gdesc->bg_inode_bitmap);
		if (bmap == nullptr)
			return 0;

		u32 inodes_in_group = sbi->inode_per_group;
		u32 base_ino = g * sbi->inode_per_group;
		if (base_ino + inodes_in_group > raw_sb->s_inode_count)
			inodes_in_group = raw_sb->s_inode_count - base_ino;

		u32 bit = 0;
		bool found = false;
		u32 nbytes = (inodes_in_group + 7) / 8;
		for (u32 byte = 0; byte < nbytes && !found; byte++) {
			u8 b = (u8)bmap->data[byte];
			if (b == 0xFF)
				continue;
			for (u32 k = 0; k < 8; k++) {
				if (b & (1u << k))
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
		u32 gdt_blk =
			raw_sb->s_first_data_block + 1 + g / gdesc_per_blk;
		u32 gdt_off = (g % gdesc_per_blk) * sizeof(struct ext2_bg_desc);
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

void ext2_free_inode(u32 ino)
{
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	if (ino == 0)
		return;

	u32 idx = ino - 1;
	u32 g = idx / sbi->inode_per_group;
	u32 bit = idx % sbi->inode_per_group;

	if (g >= sbi->group_count)
		return;

	struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

	struct blk_cache *bmap = bread(gdesc->bg_inode_bitmap);
	if (bmap == nullptr)
		return;
	u32 byte = bit / 8;
	u32 k = bit % 8;
	bmap->data[byte] = (char)((u8)bmap->data[byte] & (u8) ~(1u << k));
	bwrite(bmap);
	brelse(bmap);

	gdesc->bg_free_inodes_count++;
	u32 gdesc_per_blk = fs.block_size / sizeof(struct ext2_bg_desc);
	u32 gdt_blk = raw_sb->s_first_data_block + 1 + g / gdesc_per_blk;
	u32 gdt_off = (g % gdesc_per_blk) * sizeof(struct ext2_bg_desc);
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
