/**
 * @file inode.c
 * @brief Ext2 文件系统 inode 操作
 *
 * 本文件实现了 Ext2 文件系统的 inode 管理功能，包括 inode 的定位、
 * 读取、写入、缓存管理以及磁盘空间释放等操作。
 */

#include "ext2/ext2.h"
#include "ext2/ext2_def.h"
#include "fs.h"
#include "log.h"

/**
 * @brief 根据 inode 号定位其在磁盘上的物理位置
 *
 * 计算 inode 所在的块组和块内偏移，用于读取或写入磁盘上的 inode 数据。
 *
 * @param ino 待查询的 inode 号（从 1 开始）
 * @param[out] blk_no 输出参数，返回 inode 所在的块号
 * @param[out] offset 输出参数，返回 inode 在块内的字节偏移
 *
 * @note inode 号超出范围将触发 panic
 */
void ext2_inode_loc(const u32 ino, u32 *blk_no, u32 *offset)
{
	const auto sbi = (struct ext2_sb_info *)fs.fs_private;

	const u32 ino_idx = ino - 1;

	// 组索引，确定inode属于哪一个块组
	const u32 group = ino_idx / sbi->inode_per_group;

	if (group >= sbi->group_count)
		panic("ext2: inode %u out of range", ino);

	// 组内偏移
	const u32 local_ino = ino_idx % sbi->inode_per_group;

	struct ext2_bg_desc *gdesc = &sbi->gb_table[group];

	const u32 ino_table_offset = local_ino * sbi->inode_size;

	// 块号计算
	*blk_no = gdesc->bg_inode_table + ino_table_offset / fs.block_size;
	// 块内字节偏移
	*offset = ino_table_offset % fs.block_size;
}

/**
 * @brief 获取 inode 对象
 *
 * 首先在缓存中查找，若命中则增加引用计数；若未命中则从磁盘读取并加入缓存。
 *
 * @param ino inode 号（从 1 开始，0 为无效值）
 *
 * @return 成功返回 inode 指针；失败返回 nullptr
 *
 * @note 返回的 inode 引用计数为 1，使用完毕应调用 iput 释放
 */
struct inode *ext2_iget(const u32 ino)
{
	if (ino == 0)
		return nullptr;

	// 先查找inode缓存
	struct list_head *pos;
	list_for_each(pos, &fs.inode_cache) {
		const auto cached = list_entry(pos, struct inode, cache_link);
		if (cached->ino == ino) {
			cached->ref++;
			return cached;
		}
	}

	// 分配struct inode结构体并进行基本初始化
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

	// 出发磁盘读取获得inode数据
	if (ext2_read_inode(inode) < 0) {
		kmfree(inode->fs_private);
		kmfree(inode);
		return nullptr;
	}

	// 加入缓存链表供下次使用
	list_add(&inode->cache_link, &fs.inode_cache);
	fs.icache_size++;

	return inode;
}

/**
 * @brief 从磁盘读取 inode 数据
 *
 * 定位 inode 在磁盘上的位置，读取原始数据并填充到内存结构体中。
 * 若 inode 已标记为有效（valid），则直接返回成功。
 *
 * @param inode 目标 inode 结构体指针
 *
 * @return 成功返回 0；失败返回 -1
 */
i32 ext2_read_inode(struct inode *inode)
{
	if (inode->valid)
		return 0;

	// 定位目标inode在磁盘上的物理块号和偏移值
	u32 blk_no, offset;
	ext2_inode_loc(inode->ino, &blk_no, &offset);

	// 读取数据
	struct blk_cache *cache = bread(blk_no);
	if (cache == nullptr)
		return -1;

	// 获得原始ext2_inode
	const auto raw = (struct ext2_inode *)(cache->data + offset);

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

	// 将磁盘上的i_block数据拷贝的fs_private中
	const auto info = (struct ext2_inode_info *)inode->fs_private;
	for (i32 i = 0; i < EXT2_N_BLOCKS; i++)
		info->i_block[i] = raw->i_block[i];
	info->i_flags = raw->i_flags;

	inode->valid = true;
	brelse(cache);

	return 0;
}

/**
 * @brief 将 inode 数据写入磁盘
 *
 * 将内存中的 inode 元数据同步到磁盘上的对应位置。
 * 写入完成后清除 dirty 标志。
 *
 * @param inode 目标 inode 结构体指针
 *
 * @return 成功返回 0；失败返回 -1
 *
 * @note 无效 inode（valid == false）将直接返回失败
 */
i32 ext2_write_inode(struct inode *inode)
{
	if (inode == nullptr || !inode->valid)
		return -1;

	u32 blk_no, offset;
	ext2_inode_loc(inode->ino, &blk_no, &offset);

	struct blk_cache *cache = bread(blk_no);
	if (cache == nullptr)
		return -1;

	const auto raw = (struct ext2_inode *)(cache->data + offset);
	const auto info = (struct ext2_inode_info *)inode->fs_private;

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

/**
 * @brief 销毁 inode 结构体
 *
 * 释放 inode 占用的内存资源，包括私有数据区域和结构体本身。
 *
 * @param inode 目标 inode 结构体指针
 *
 * @note 空指针将被安全忽略
 */
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

/**
 * @brief Ext2 inode 操作表
 *
 * 注册到 VFS 层的 inode 操作接口实现。
 */
struct inode_ops ext2_inode_ops = {
	.read_inode = ext2_read_inode,
	.write_inode = ext2_write_inode,
	.destroy_inode = ext2_destroy_inode,
	.lookup = ext2_lookup,
	.create = ext2_create,
	.mkdir = ext2_mkdir,
	.unlink = ext2_unlink,
	.truncate = ext2_truncate_inode,
	.free_inode = ext2_free_inode,
};

/**
 * @brief 递归释放块及其子块
 *
 * 释放指定块号及其所有间接索引子块。通过递归深度控制释放层级：
 * - level = 0: 直接块，仅释放自身
 * - level = 1: 一级间接索引块，先释放子块再释放自身
 * - level = 2: 二级间接索引块，递归释放
 * - level = 3: 三级间接索引块，递归释放
 *
 * @param block_nr 待释放的块号
 * @param level 递归深度（0 为直接块，1-3 为间接索引层级）
 */
static void ext2_free_branch(const u32 block_nr, const i32 level)
{
	if (block_nr == 0)
		return;

	if (level > 0) {
		struct blk_cache *cache = bread(block_nr);
		if (cache != nullptr) {
			const auto ptrs = (u32 *)cache->data;
			const u32 ptrs_per_blk = fs.block_size / 4;

			for (u32 i = 0; i < ptrs_per_blk; i++) {
				if (ptrs[i] != 0) {
					ext2_free_branch(ptrs[i], level - 1);
				}
			}
			brelse(cache);
		}
	}

	ext2_free_block(block_nr);
}

/**
 * @brief 截断 inode 释放所有磁盘块
 *
 * 释放 inode 关联的所有数据块，包括：
 * - 直接块（0-11）
 * - 一级间接索引块及其子块
 * - 二级间接索引块及其子块
 * - 三级间接索引块及其子块
 *
 * 截断后 inode 大小归零，并同步更新到磁盘。
 *
 * @param inode 目标 inode 结构体指针
 *
 * @note 空指针将被安全忽略
 */
void ext2_truncate_inode(struct inode *inode)
{
	if (inode == nullptr)
		return;

	auto info = (struct ext2_inode_info *)inode->fs_private;

	for (i32 i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		ext2_free_branch(info->i_block[i], 0);
		info->i_block[i] = 0;
	}

	ext2_free_branch(info->i_block[EXT2_IND_BLOCK], 1);
	info->i_block[EXT2_IND_BLOCK] = 0;

	ext2_free_branch(info->i_block[EXT2_DIND_BLOCK], 2);
	info->i_block[EXT2_DIND_BLOCK] = 0;

	ext2_free_branch(info->i_block[EXT2_TIND_BLOCK], 3);
	info->i_block[EXT2_TIND_BLOCK] = 0;

	inode->size = 0;
	inode->dirty = true;
	ext2_write_inode(inode);
}
