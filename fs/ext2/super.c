#include "ext2.h"
#include "fs.h"
#include "utils.h"

#define EXT2_GDESC_SIZE(c) ((c) * sizeof(struct ext2_bg_desc))

i32 ext2_fs_init(void)
{
	struct blk_cache *cache = bread(EXT2_SUPER_BLKNUM);
	if (cache == nullptr)
		return -1;

	const auto sp =
		(struct ext2_super_block *)(cache->data + EXT2_SUPER_OFFSET);
	if (sp->s_magic != EXT2_SUPER_MAGIC) {
		brelse(cache);
		return -2;
	}

	struct ext2_sb_info *ext2_info = kmalloc(sizeof(struct ext2_sb_info));
	if (ext2_info == nullptr) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	ext2_info->sb_buf = cache;
	ext2_info->raw_sb =
		(struct ext2_super_block *)(cache->data + EXT2_SUPER_OFFSET);

	if (sp->s_log_block_size > EXT2_MAX_LOG_BLOCK_SIZE) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	const u32 block_size = 1024U << sp->s_log_block_size;

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

	ext2_info->inode_per_group = sp->s_inodes_per_group;
	ext2_info->inode_size = (sp->s_rev_level >= EXT2_DYNAMIC_REV) ?
					(u32)sp->s_inode_size :
					EXT2_GOOD_OLD_INODE_SIZE;

	const usize gdesc_size = EXT2_GDESC_SIZE(ext2_info->group_count);
	ext2_info->gb_table = kmalloc(gdesc_size);
	if (ext2_info->gb_table == nullptr) {
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

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

	{
	}
	fs.block_size = block_size;
	fs.blocks_count = sp->s_blocks_count;
	fs.inode_ops = &ext2_inode_ops;
	fs.file_ops = &ext2_file_ops;
	fs.fs_private = ext2_info;
	list_init(&fs.inode_cache);
	fs.icache_size = 0;
	fs.dev = 0;

	fs.root = ext2_iget(EXT2_ROOT_INO);
	if (fs.root == nullptr) {
		kmfree(ext2_info->gb_table);
		brelse(cache);
		kmfree(ext2_info);
		return -1;
	}

	return 0;
}
