#include "ext2.h"
#include "fs.h"
#include "log.h"

void ext2_inode_loc(u32 ino, u32 *blk_no, u32 *offset)
{
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;

	u32 ino_idx = ino - 1;

	u32 group = ino_idx / sbi->inode_per_group;

	if (group >= sbi->group_count)
		panic("ext2: inode %u out of range", ino);

	u32 local_ino = ino_idx % sbi->inode_per_group;

	struct ext2_bg_desc *gdesc = &sbi->gb_table[group];

	u32 ino_table_offset = local_ino * sbi->inode_size;

	*blk_no = gdesc->bg_inode_table + ino_table_offset / fs.block_size;
	*offset = ino_table_offset % fs.block_size;
}

struct inode *ext2_iget(u32 ino)
{
	if (ino == 0)
		return nullptr;

	struct list_head *pos;
	list_for_each(pos, &fs.inode_cache) {
		struct inode *cached =
			list_entry(pos, struct inode, cache_link);
		if (cached->ino == ino) {
			cached->ref++;
			return cached;
		}
	}

	struct inode *inode = kmalloc(sizeof(struct inode));
	if (inode == nullptr)
		return nullptr;

	inode->ino = ino;
	inode->ref = 1;
	inode->valid = false;
	inode->dirty = false;
	inode->fs = &fs;
	inode->iops = &ext2_inode_ops;
	inode->fops = &ext2_file_ops;

	inode->fs_private = kmalloc(sizeof(struct ext2_inode_info));
	if (inode->fs_private == nullptr) {
		kmfree(inode);
		return nullptr;
	}

	list_init(&inode->cache_link);

	if (ext2_read_inode(inode) < 0) {
		kmfree(inode->fs_private);
		kmfree(inode);
		return nullptr;
	}

	list_add(&inode->cache_link, &fs.inode_cache);
	fs.icache_size++;

	return inode;
}

i32 ext2_read_inode(struct inode *inode)
{
	if (inode->valid)
		return 0;

	u32 blk_no, offset;
	ext2_inode_loc(inode->ino, &blk_no, &offset);

	struct blk_cache *cache = bread(blk_no);
	if (cache == nullptr)
		return -1;

	struct ext2_inode *raw = (struct ext2_inode *)(cache->data + offset);

	inode->mode = raw->i_mode;
	inode->uid = raw->i_uid;
	inode->gid = raw->i_gid;
	inode->size = raw->i_size;
	inode->nlinks = raw->i_links_count;

	if (S_ISDIR(raw->i_mode))
		inode->type = INODE_DIR;
	else if (S_ISREG(raw->i_mode))
		inode->type = INODE_FILE;
	else if (S_ISLNK(raw->i_mode))
		inode->type = INODE_SYNLINK;
	else
		inode->type = INODE_FILE;

	struct ext2_inode_info *info =
		(struct ext2_inode_info *)inode->fs_private;
	for (i32 i = 0; i < EXT2_N_BLOCKS; i++)
		info->i_block[i] = raw->i_block[i];
	info->i_flags = raw->i_flags;

	inode->valid = true;
	brelse(cache);

	return 0;
}

i32 ext2_write_inode(struct inode *inode)
{
	if (inode == nullptr || !inode->valid)
		return -1;

	u32 blk_no, offset;
	ext2_inode_loc(inode->ino, &blk_no, &offset);

	struct blk_cache *cache = bread(blk_no);
	if (cache == nullptr)
		return -1;

	struct ext2_inode *raw = (struct ext2_inode *)(cache->data + offset);
	struct ext2_inode_info *info =
		(struct ext2_inode_info *)inode->fs_private;

	raw->i_mode = (u16)inode->mode;
	raw->i_uid = (u16)inode->uid;
	raw->i_gid = (u16)inode->gid;
	raw->i_size = inode->size;
	raw->i_links_count = (u16)inode->nlinks;

	u32 used_blocks = 0;
	for (i32 i = 0; i < EXT2_N_BLOCKS; i++) {
		if (info->i_block[i] != 0)
			used_blocks++;
	}
	raw->i_blocks = used_blocks * (fs.block_size / 512);

	for (i32 i = 0; i < EXT2_N_BLOCKS; i++)
		raw->i_block[i] = info->i_block[i];
	raw->i_flags = info->i_flags;

	bwrite(cache);
	brelse(cache);

	inode->dirty = false;
	return 0;
}

void ext2_destroy_inode(struct inode *inode)
{
	if (inode == nullptr)
		return;
	if (inode->fs_private != nullptr) {
		kmfree(inode->fs_private);
		inode->fs_private = nullptr;
	}
	kmfree(inode);
}

struct inode_ops ext2_inode_ops = {
	.read_inode = ext2_read_inode,
	.write_inode = ext2_write_inode,
	.destroy_inode = ext2_destroy_inode,
	.lookup = ext2_lookup,
	.create = ext2_create,
	.mkdir = ext2_mkdir,
	.unlink = ext2_unlink,
};

void ext2_truncate_inode(struct inode *inode)
{
	if (inode == nullptr)
		return;

	struct ext2_inode_info *info =
		(struct ext2_inode_info *)inode->fs_private;
	u32 ptrs_per_blk = fs.block_size / 4;

	for (u32 i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		if (info->i_block[i] != 0) {
			ext2_free_block(info->i_block[i]);
			info->i_block[i] = 0;
		}
	}

	if (info->i_block[EXT2_IND_BLOCK] != 0) {
		struct blk_cache *ind = bread(info->i_block[EXT2_IND_BLOCK]);
		if (ind != nullptr) {
			u32 *ptrs = (u32 *)ind->data;
			for (u32 i = 0; i < ptrs_per_blk; i++) {
				if (ptrs[i] != 0)
					ext2_free_block(ptrs[i]);
			}
			brelse(ind);
		}
		ext2_free_block(info->i_block[EXT2_IND_BLOCK]);
		info->i_block[EXT2_IND_BLOCK] = 0;
	}

	if (info->i_block[EXT2_DIND_BLOCK] != 0) {
		struct blk_cache *dind = bread(info->i_block[EXT2_DIND_BLOCK]);
		if (dind != nullptr) {
			u32 *dptrs = (u32 *)dind->data;
			for (u32 i = 0; i < ptrs_per_blk; i++) {
				if (dptrs[i] == 0)
					continue;
				struct blk_cache *ind = bread(dptrs[i]);
				if (ind != nullptr) {
					u32 *iptrs = (u32 *)ind->data;
					for (u32 j = 0; j < ptrs_per_blk; j++) {
						if (iptrs[j] != 0)
							ext2_free_block(
								iptrs[j]);
					}
					brelse(ind);
				}
				ext2_free_block(dptrs[i]);
			}
			brelse(dind);
		}
		ext2_free_block(info->i_block[EXT2_DIND_BLOCK]);
		info->i_block[EXT2_DIND_BLOCK] = 0;
	}

	inode->size = 0;
	inode->dirty = true;
	ext2_write_inode(inode);
}
