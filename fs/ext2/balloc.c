#include "ext2.h"
#include "fs.h"

u32 ext2_alloc_block(void)
{
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;
	struct ext2_super_block *raw_sb = sbi->raw_sb;

	for (u32 g = 0; g < sbi->group_count; g++) {
		struct ext2_bg_desc *gdesc = &sbi->gb_table[g];

		if (gdesc->bg_free_blocks_count == 0)
			continue;

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
			u8 b = (u8)bmap->data[byte];
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

		raw_sb->s_free_blocks_count--;
		bwrite(sbi->sb_buf);

		return base + bit;
	}

	return 0;
}

void ext2_free_block(u32 blk_no)
{
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;
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
	u32 byte = bit / 8;
	u32 k = bit % 8;
	bmap->data[byte] = (char)((u8)bmap->data[byte] & (u8) ~(1u << k));
	bwrite(bmap);
	brelse(bmap);

	gdesc->bg_free_blocks_count++;
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

	raw_sb->s_free_blocks_count++;
	bwrite(sbi->sb_buf);
}
