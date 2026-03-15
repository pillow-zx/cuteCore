#include "fs.h"
#include "ext2.h"
#include "utils.h"

/*
 * ext2_file_open - 打开文件
 *
 * 此函数本次任务中仅需定义，无需进一步实现
 */
__maybe_unused i32 ext2_file_open(__maybe_unused struct inode *inode, __maybe_unused struct file *file)
{
	/* TODO: 实现文件打开 */
	return 0;
}

/*
 * ext2_file_release - 关闭文件
 *
 * 此函数本次任务中仅需定义，无需进一步实现
 */
__maybe_unused void ext2_file_release(__maybe_unused struct inode *inode, __maybe_unused struct file *file)
{
	/* TODO: 实现文件关闭 */
}

/*
 * ext2_read_block - 读取 inode 的指定逻辑块
 *
 * @inode: 目标 inode
 * @lbk: 逻辑块号
 * @buf: 输出缓冲区 (至少 block_size 字节)
 *
 * 返回: 0 成功，负数失败
 */
static i32 ext2_read_block(struct inode *inode, u32 lbk, char *buf)
{
	struct ext2_inode_info *info =
		(struct ext2_inode_info *)inode->fs_private;

	/* 直接块 */
	if (lbk < EXT2_NDIR_BLOCKS) {
		u32 blk_no = info->i_block[lbk];
		if (blk_no == 0) {
			/* 稀疏文件，返回零填充 */
			memset(buf, 0, fs.block_size);
			return 0;
		}
		struct blk_cache *cache = bread(blk_no);
		if (cache == nullptr)
			return -1;
		memcpy(buf, cache->data, fs.block_size);
		brelse(cache);
		return 0;
	}

	/* 一级间接块 */
	lbk -= EXT2_NDIR_BLOCKS;
	if (lbk < fs.block_size / 4) {
		u32 ind_blk = info->i_block[EXT2_IND_BLOCK];
		if (ind_blk == 0) {
			memset(buf, 0, fs.block_size);
			return 0;
		}
		struct blk_cache *ind_cache = bread(ind_blk);
		if (ind_cache == nullptr)
			return -1;

		u32 blk_no = ((u32 *)ind_cache->data)[lbk];
		brelse(ind_cache);

		if (blk_no == 0) {
			memset(buf, 0, fs.block_size);
			return 0;
		}

		struct blk_cache *data_cache = bread(blk_no);
		if (data_cache == nullptr)
			return -1;
		memcpy(buf, data_cache->data, fs.block_size);
		brelse(data_cache);
		return 0;
	}

	/* 二级间接块 */
	lbk -= fs.block_size / 4;
	u32 ptrs_per_blk = fs.block_size / 4;
	if (lbk < ptrs_per_blk * ptrs_per_blk) {
		u32 dind_blk = info->i_block[EXT2_DIND_BLOCK];
		if (dind_blk == 0) {
			memset(buf, 0, fs.block_size);
			return 0;
		}

		u32 ind_idx = lbk / ptrs_per_blk;
		u32 dblk_idx = lbk % ptrs_per_blk;

		struct blk_cache *dind_cache = bread(dind_blk);
		if (dind_cache == nullptr)
			return -1;

		u32 ind_blk = ((u32 *)dind_cache->data)[ind_idx];
		brelse(dind_cache);

		if (ind_blk == 0) {
			memset(buf, 0, fs.block_size);
			return 0;
		}

		struct blk_cache *ind_cache = bread(ind_blk);
		if (ind_cache == nullptr)
			return -1;

		u32 blk_no = ((u32 *)ind_cache->data)[dblk_idx];
		brelse(ind_cache);

		if (blk_no == 0) {
			memset(buf, 0, fs.block_size);
			return 0;
		}

		struct blk_cache *data_cache = bread(blk_no);
		if (data_cache == nullptr)
			return -1;
		memcpy(buf, data_cache->data, fs.block_size);
		brelse(data_cache);
		return 0;
	}

	/* 三级间接块 - 本次任务暂不实现 */
	return -1;
}

/*
 * ext2_file_read - 读取文件内容
 *
 * @file: 文件结构
 * @buf: 用户缓冲区
 * @len: 读取长度
 * @offset: 文件偏移量指针（会被更新）
 *
 * 返回: 实际读取的字节数，负数表示错误
 */
i32 ext2_file_read(struct file *file, void *buf, usize len, off_t *offset)
{
	struct inode *inode = file->inode;

	/* 确保运行时块大小不超过编译时常量 */
	if (fs.block_size > BLKSZ)
		return -1;

	if (*offset >= inode->size)
		return 0;

	/* 计算实际可读取的字节数 */
	if (*offset + len > inode->size)
		len = inode->size - *offset;

	usize total = 0;
	char block_buf[BLKSZ];

	while (total < len) {
		/* 计算当前块号和块内偏移 */
		u32 lbk = (*offset) / fs.block_size;
		u32 blk_off = (*offset) % fs.block_size;

		/* 读取块数据 */
		if (ext2_read_block(inode, lbk, block_buf) < 0)
			return total > 0 ? (i32)total : -1;

		/* 计算本次拷贝的字节数 */
		usize chunk = fs.block_size - blk_off;
		if (chunk > len - total)
			chunk = len - total;

		/* 拷贝到用户缓冲区 */
		memcpy((char *)buf + total, block_buf + blk_off, chunk);

		total += chunk;
		*offset += chunk;
	}

	return (i32)total;
}

/*
 * ext2_write_block - 将数据写入 inode 的指定逻辑块
 *
 * @inode: 目标 inode
 * @lbk:   逻辑块号（相对于文件起始）
 * @buf:   源缓冲区（fs.block_size 字节）
 *
 * 若该逻辑块尚未分配（稀疏或文件末尾），则自动分配新块并在
 * 间接索引链中记录。间接块本身（一级/二级）若也未分配则同样
 * 由本函数分配并清零。
 *
 * 返回: 0 成功，-1 失败
 *
 * 实现范围：直接块、一级间接、二级间接。三级间接不支持（返回 -1）。
 */
static i32 ext2_write_block(struct inode *inode, u32 lbk, const char *buf)
{
	struct ext2_inode_info *info =
		(struct ext2_inode_info *)inode->fs_private;

	/* ── 直接块 (lbk < 12) ─────────────────────────────────────── */
	if (lbk < EXT2_NDIR_BLOCKS) {
		if (info->i_block[lbk] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0)
				return -1;
			info->i_block[lbk] = nb;
			inode->dirty = true;
		}
		struct blk_cache *c = bread(info->i_block[lbk]);
		if (c == nullptr)
			return -1;
		memcpy(c->data, buf, fs.block_size);
		bwrite(c);
		brelse(c);
		return 0;
	}

	u32 ptrs_per_blk = fs.block_size / 4;

	/* ── 一级间接块 ─────────────────────────────────────────────── */
	lbk -= EXT2_NDIR_BLOCKS;
	if (lbk < ptrs_per_blk) {
		/* 如果间接块本身不存在，先分配并清零 */
		if (info->i_block[EXT2_IND_BLOCK] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0)
				return -1;
			info->i_block[EXT2_IND_BLOCK] = nb;
			inode->dirty = true;
			/* 清零间接块 */
			struct blk_cache *ic = bread(nb);
			if (ic == nullptr)
				return -1;
			memset(ic->data, 0, fs.block_size);
			bwrite(ic);
			brelse(ic);
		}

		struct blk_cache *ind = bread(info->i_block[EXT2_IND_BLOCK]);
		if (ind == nullptr)
			return -1;

		u32 *ptrs = (u32 *)ind->data;
		if (ptrs[lbk] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0) {
				brelse(ind);
				return -1;
			}
			ptrs[lbk] = nb;
			bwrite(ind);
		}
		u32 data_blk = ptrs[lbk];
		brelse(ind);

		struct blk_cache *c = bread(data_blk);
		if (c == nullptr)
			return -1;
		memcpy(c->data, buf, fs.block_size);
		bwrite(c);
		brelse(c);
		return 0;
	}

	/* ── 二级间接块 ─────────────────────────────────────────────── */
	lbk -= ptrs_per_blk;
	if (lbk < ptrs_per_blk * ptrs_per_blk) {
		/* 分配二级间接块（若不存在） */
		if (info->i_block[EXT2_DIND_BLOCK] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0)
				return -1;
			info->i_block[EXT2_DIND_BLOCK] = nb;
			inode->dirty = true;
			struct blk_cache *dc = bread(nb);
			if (dc == nullptr)
				return -1;
			memset(dc->data, 0, fs.block_size);
			bwrite(dc);
			brelse(dc);
		}

		u32 ind_idx = lbk / ptrs_per_blk;
		u32 dblk_idx = lbk % ptrs_per_blk;

		struct blk_cache *dind = bread(info->i_block[EXT2_DIND_BLOCK]);
		if (dind == nullptr)
			return -1;
		u32 *dptrs = (u32 *)dind->data;

		/* 分配一级间接块（若不存在） */
		if (dptrs[ind_idx] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0) {
				brelse(dind);
				return -1;
			}
			dptrs[ind_idx] = nb;
			bwrite(dind);
			struct blk_cache *ic = bread(nb);
			if (ic == nullptr) {
				brelse(dind);
				return -1;
			}
			memset(ic->data, 0, fs.block_size);
			bwrite(ic);
			brelse(ic);
		}
		u32 ind_blk = dptrs[ind_idx];
		brelse(dind);

		struct blk_cache *ind = bread(ind_blk);
		if (ind == nullptr)
			return -1;
		u32 *iptrs = (u32 *)ind->data;

		if (iptrs[dblk_idx] == 0) {
			u32 nb = ext2_alloc_block();
			if (nb == 0) {
				brelse(ind);
				return -1;
			}
			iptrs[dblk_idx] = nb;
			bwrite(ind);
		}
		u32 data_blk = iptrs[dblk_idx];
		brelse(ind);

		struct blk_cache *c = bread(data_blk);
		if (c == nullptr)
			return -1;
		memcpy(c->data, buf, fs.block_size);
		bwrite(c);
		brelse(c);
		return 0;
	}

	/* 三级间接块：暂不支持 */
	return -1;
}

/*
 * ext2_file_write - 写入文件内容
 *
 * @file:   文件结构
 * @buf:    用户缓冲区（源数据）
 * @len:    写入字节数
 * @offset: 文件偏移量指针（写入完成后被更新）
 *
 * 对于每个跨越的逻辑块，先读出原有数据（read-modify-write），
 * 将用户数据覆盖到对应区域，再写回。若写入超过文件末尾则自动
 * 扩展 inode->size，并在完成后调用 ext2_write_inode 写回 inode。
 *
 * 返回: 实际写入字节数，负数表示错误
 */
i32 ext2_file_write(struct file *file, const void *buf, usize len,
		    off_t *offset)
{
	struct inode *inode = file->inode;

	if (fs.block_size > BLKSZ)
		return -1;
	if (len == 0)
		return 0;

	usize total = 0;
	char block_buf[BLKSZ];

	while (total < len) {
		u32 lbk = (*offset) / fs.block_size;
		u32 blk_off = (*offset) % fs.block_size;

		/*
		 * 读-改-写：先读出已有数据（可能是全零的新块），
		 * 再将用户数据覆盖到块内对应偏移处，然后写回整块。
		 */
		if (ext2_read_block(inode, lbk, block_buf) < 0) {
			/*
			 * 对于文件末尾之后的新块，read_block 的稀疏路径
			 * 会返回零填充的缓冲区并返回 0。只有真正的 I/O
			 * 错误才会返回 -1。
			 */
			return total > 0 ? (i32)total : -1;
		}

		usize chunk = fs.block_size - blk_off;
		if (chunk > len - total)
			chunk = len - total;

		memcpy(block_buf + blk_off, (const char *)buf + total, chunk);

		if (ext2_write_block(inode, lbk, block_buf) < 0)
			return total > 0 ? (i32)total : -1;

		total += chunk;
		*offset += chunk;
	}

	/* 如果写入超过了原文件末尾，更新 inode 大小 */
	if (*offset > inode->size) {
		inode->size = (u32)*offset;
		inode->dirty = true;
	}

	/* 写回 inode */
	if (inode->dirty)
		ext2_write_inode(inode);

	return (i32)total;
}

/*
 * ext2_lookup - 在目录中查找指定名称的文件
 *
 * @dir:     目录 inode
 * @name:    要查找的文件名
 * @namelen: 文件名长度
 *
 * 遍历目录的所有数据块，逐条解析 ext2_dir_entry，比较文件名。
 *
 * 返回: 找到则返回对应 inode（引用计数已增），否则返回 nullptr
 */
struct inode *ext2_lookup(struct inode *dir, const char *name, i32 namelen)
{
	if (dir == nullptr || dir->type != INODE_DIR)
		return nullptr;
	if (name == nullptr || namelen <= 0)
		return nullptr;

	u32 block_size = fs.block_size;
	u32 nblocks = (dir->size + block_size - 1) / block_size;
	char block_buf[BLKSZ];

	for (u32 lbk = 0; lbk < nblocks; lbk++) {
		if (ext2_read_block(dir, lbk, block_buf) < 0)
			continue;

		u32 pos = 0;
		while (pos + sizeof(struct ext2_dir_entry) <= block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(block_buf + pos);

			if (de->rec_len == 0)
				break;

			if (de->inode != 0 && de->name_len == (u8)namelen &&
			    memcmp(de->name, name, (usize)namelen) == 0) {
				return ext2_iget(de->inode);
			}

			pos += de->rec_len;
		}
	}

	return nullptr;
}

/*
 * ext2_dir_add_entry - 向目录追加一条目录项
 *
 * 在目录已有数据块中寻找可以容纳新条目的空间（利用末尾条目的
 * rec_len 填充区）；若没有空间则分配一个新的数据块。
 *
 * @dir:     目录 inode
 * @name:    文件名
 * @namelen: 文件名长度
 * @ino:     新条目的 inode 编号
 * @ftype:   文件类型（1=普通文件，2=目录）
 *
 * 返回: 0 成功，-1 失败
 */
static i32 ext2_dir_add_entry(struct inode *dir, const char *name, u32 namelen,
			      u32 ino, u8 ftype)
{
	u32 block_size = fs.block_size;
	/* 新条目所需的最小 rec_len（名称后按 4 字节对齐） */
	u32 needed = (u32)(sizeof(struct ext2_dir_entry) + namelen);
	needed = (needed + 3) & ~3u;

	u32 nblocks = (dir->size + block_size - 1) / block_size;
	char block_buf[BLKSZ];

	for (u32 lbk = 0; lbk < nblocks; lbk++) {
		if (ext2_read_block(dir, lbk, block_buf) < 0)
			continue;

		u32 pos = 0;
		while (pos + sizeof(struct ext2_dir_entry) <= block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(block_buf + pos);

			if (de->rec_len == 0)
				break;

			/* 计算该条目实际占用的最小空间 */
			u32 actual = (u32)(sizeof(struct ext2_dir_entry) +
					   de->name_len);
			actual = (actual + 3) & ~3u;
			u32 slack = de->rec_len - actual;

			if (slack >= needed) {
				/*
				 * 将末尾的空闲空间分割出来：
				 * 缩短当前条目的 rec_len 到 actual，
				 * 在 actual 偏移处写入新条目，其 rec_len
				 * 占用所有剩余空间。
				 */
				de->rec_len = (u16)actual;

				struct ext2_dir_entry *ne =
					(struct ext2_dir_entry *)(block_buf +
								  pos + actual);
				ne->inode = ino;
				ne->rec_len = (u16)(slack);
				ne->name_len = (u8)namelen;
				ne->file_type = ftype;
				memcpy(ne->name, name, namelen);

				/* 写回该目录块 */
				return ext2_write_block(dir, lbk, block_buf);
			}

			pos += de->rec_len;
		}
	}

	/*
	 * 没有空间，分配新目录块。
	 * ext2_write_block 会自动分配物理块。
	 */
	memset(block_buf, 0, block_size);
	struct ext2_dir_entry *ne = (struct ext2_dir_entry *)block_buf;
	ne->inode = ino;
	ne->rec_len = (u16)block_size; /* 占满整个块 */
	ne->name_len = (u8)namelen;
	ne->file_type = ftype;
	memcpy(ne->name, name, namelen);

	u32 lbk = nblocks; /* 新块的逻辑块号 */
	if (ext2_write_block(dir, lbk, block_buf) < 0)
		return -1;

	dir->size += block_size;
	dir->dirty = true;
	ext2_write_inode(dir);
	return 0;
}

/*
 * ext2_create - 在目录中创建一个新的普通文件
 *
 * @dir:     父目录 inode
 * @name:    新文件名
 * @namelen: 文件名长度
 * @mode:    文件权限位（将自动附加 S_IFREG）
 *
 * 步骤：
 *   1. 分配一个新的 inode 编号
 *   2. 初始化磁盘上的 struct ext2_inode（清零，填写 mode/links）
 *   3. 在父目录中追加目录项
 *   4. 调用 ext2_iget 加载并返回新 inode
 *
 * 返回: 成功返回新 inode，失败返回 nullptr
 */
struct inode *ext2_create(struct inode *dir, const char *name, i32 namelen,
			  u32 mode)
{
	if (dir == nullptr || name == nullptr || namelen <= 0)
		return nullptr;

	/* 分配 inode 编号 */
	u32 ino = ext2_alloc_inode();
	if (ino == 0)
		return nullptr;

	/* 初始化磁盘上的 inode：清零整个 inode 大小的区域，写入 mode */
	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;
	u32 blk_no, off;
	ext2_inode_loc(ino, &blk_no, &off);

	struct blk_cache *cache = bread(blk_no);
	if (cache == nullptr) {
		ext2_free_inode(ino);
		return nullptr;
	}

	/* 清零该 inode 在块中的区域 */
	memset(cache->data + off, 0, sbi->inode_size);

	/* 填写必要字段 */
	struct ext2_inode *raw = (struct ext2_inode *)(cache->data + off);
	raw->i_mode = (u16)(S_IFREG | (mode & 0xFFFu));
	raw->i_links_count = 1;

	bwrite(cache);
	brelse(cache);

	/* 在父目录中添加条目（file_type=1 表示普通文件） */
	if (ext2_dir_add_entry(dir, name, (u32)namelen, ino, 1) < 0) {
		ext2_free_inode(ino);
		return nullptr;
	}

	/* 加载并返回新 inode */
	return ext2_iget(ino);
}

/*
 * ext2_mkdir - 在目录中创建一个新的子目录
 *
 * @dir:     父目录 inode
 * @name:    新目录名
 * @namelen: 目录名长度
 * @mode:    权限位（自动附加 S_IFDIR）
 *
 * 步骤：
 *   1. 分配新 inode（类型 S_IFDIR，i_links_count=2：自身 "." 和父目录项）
 *   2. 分配一个数据块，写入 "." 和 ".." 两条目录项
 *   3. 在父目录中添加目录项（file_type=2）
 *   4. 递增父目录 nlinks（因为 ".." 指向父目录）并写回父 inode
 *   5. 更新块组描述符的 bg_used_dirs_count
 *
 * 返回: 成功返回新目录 inode，失败返回 nullptr
 */
struct inode *ext2_mkdir(struct inode *dir, const char *name, i32 namelen,
			 u32 mode)
{
	if (dir == nullptr || name == nullptr || namelen <= 0)
		return nullptr;

	struct ext2_sb_info *sbi = (struct ext2_sb_info *)fs.fs_private;

	/* ── 1. 分配新 inode ──────────────────────────────────────── */
	u32 ino = ext2_alloc_inode();
	if (ino == 0)
		return nullptr;

	/* 初始化磁盘上的 inode */
	u32 blk_no, off;
	ext2_inode_loc(ino, &blk_no, &off);

	struct blk_cache *icache = bread(blk_no);
	if (icache == nullptr) {
		ext2_free_inode(ino);
		return nullptr;
	}

	memset(icache->data + off, 0, sbi->inode_size);
	struct ext2_inode *raw = (struct ext2_inode *)(icache->data + off);
	raw->i_mode = (u16)(S_IFDIR | (mode & 0xFFFu));
	raw->i_links_count = 2; /* "." + 父目录中的条目 */
	raw->i_size = fs.block_size;
	raw->i_blocks = fs.block_size / 512;

	/* ── 2. 分配数据块，写入 "." 和 ".." ─────────────────────── */
	u32 data_blk = ext2_alloc_block();
	if (data_blk == 0) {
		brelse(icache);
		ext2_free_inode(ino);
		return nullptr;
	}
	raw->i_block[0] = data_blk;
	bwrite(icache);
	brelse(icache);

	/* 构造 "." 和 ".." 目录项 */
	struct blk_cache *dcache = bread(data_blk);
	if (dcache == nullptr) {
		ext2_free_block(data_blk);
		ext2_free_inode(ino);
		return nullptr;
	}
	memset(dcache->data, 0, fs.block_size);

	/*
	 * "." 条目：inode = 新目录自身
	 * rec_len = sizeof(header) + 1 name byte，对齐到 4 字节 = 12
	 */
	u32 dot_rec = (u32)(sizeof(struct ext2_dir_entry) + 1);
	dot_rec = (dot_rec + 3) & ~3u;

	struct ext2_dir_entry *dot = (struct ext2_dir_entry *)dcache->data;
	dot->inode = ino;
	dot->rec_len = (u16)dot_rec;
	dot->name_len = 1;
	dot->file_type = 2; /* EXT2_FT_DIR */
	dot->name[0] = '.';

	/*
	 * ".." 条目：inode = 父目录
	 * rec_len 占满整个块的剩余部分
	 */
	struct ext2_dir_entry *dotdot =
		(struct ext2_dir_entry *)(dcache->data + dot_rec);
	dotdot->inode = dir->ino;
	dotdot->rec_len = (u16)(fs.block_size - dot_rec);
	dotdot->name_len = 2;
	dotdot->file_type = 2; /* EXT2_FT_DIR */
	dotdot->name[0] = '.';
	dotdot->name[1] = '.';

	bwrite(dcache);
	brelse(dcache);

	/* ── 3. 在父目录中添加条目（file_type=2 表示目录）──────────── */
	if (ext2_dir_add_entry(dir, name, (u32)namelen, ino, 2) < 0) {
		ext2_free_block(data_blk);
		ext2_free_inode(ino);
		return nullptr;
	}

	/* ── 4. 递增父目录 nlinks（".." 反向引用）────────────────── */
	dir->nlinks++;
	dir->dirty = true;
	ext2_write_inode(dir);

	/* ── 5. 更新块组描述符的 bg_used_dirs_count ──────────────── */
	u32 group = (ino - 1) / sbi->inode_per_group;
	struct ext2_bg_desc *gdesc = &sbi->gb_table[group];
	gdesc->bg_used_dirs_count++;

	/*
	 * 块组描述符表紧跟在超级块块之后（偏移 EXT2_GDESC_OFFSET）。
	 * 对于 4 KiB 块大小，超级块在块 0，GDT 也在块 0（偏移 2048）。
	 */
	struct blk_cache *sb_blk = bread(0);
	if (sb_blk != nullptr) {
		struct ext2_bg_desc *on_disk =
			(struct ext2_bg_desc *)(sb_blk->data +
						EXT2_GDESC_OFFSET);
		on_disk[group].bg_used_dirs_count = gdesc->bg_used_dirs_count;
		bwrite(sb_blk);
		brelse(sb_blk);
	}

	/* ── 6. 加载并返回新目录 inode ───────────────────────────── */
	return ext2_iget(ino);
}

/*
 * ext2_unlink - 从目录中删除一条目录项（减少硬链接计数）
 *
 * @dir:     父目录 inode
 * @name:    要删除的文件名
 * @namelen: 文件名长度
 *
 * 步骤：
 *   1. 遍历目录数据块，找到匹配的目录项
 *   2. 将该条目的 inode 字段置 0（标记为已删除），并将其空间
 *      合并到前一条目的 rec_len 中（或若为块首条目则直接清零）
 *   3. 写回修改后的目录块
 *   4. 通过 ext2_iget 加载目标 inode，递减 nlinks 并写回；
 *      若 nlinks 降为 0，iput 会调用 ext2_truncate_inode 释放块
 *
 * 返回: 0 成功，-1 失败（未找到）
 */
i32 ext2_unlink(struct inode *dir, const char *name, i32 namelen)
{
	if (dir == nullptr || name == nullptr || namelen <= 0)
		return -1;

	u32 block_size = fs.block_size;
	u32 nblocks = (dir->size + block_size - 1) / block_size;
	char block_buf[BLKSZ];

	for (u32 lbk = 0; lbk < nblocks; lbk++) {
		if (ext2_read_block(dir, lbk, block_buf) < 0)
			continue;

		u32 pos = 0;
		struct ext2_dir_entry *prev = nullptr;

		while (pos + sizeof(struct ext2_dir_entry) <= block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(block_buf + pos);

			if (de->rec_len == 0)
				break;

			if (de->inode != 0 && de->name_len == (u8)namelen &&
			    memcmp(de->name, name, (usize)namelen) == 0) {
				u32 target_ino = de->inode;

				/*
				 * 删除策略：将此条目的 rec_len 合并给前一条目；
				 * 若是块内第一条，则仅清零 inode 字段。
				 */
				if (prev != nullptr) {
					prev->rec_len = (u16)(prev->rec_len +
							      de->rec_len);
				} else {
					de->inode = 0;
				}

				/* 写回目录块 */
				if (ext2_write_block(dir, lbk, block_buf) < 0)
					return -1;

				/* 递减目标 inode 的硬链接计数 */
				struct inode *victim = ext2_iget(target_ino);
				if (victim != nullptr) {
					if (victim->nlinks > 0)
						victim->nlinks--;
					victim->dirty = true;
					ext2_write_inode(victim);
					iput(victim);
				}

				return 0;
			}

			prev = de;
			pos += de->rec_len;
		}
	}

	return -1; /* 未找到 */
}

/*
 * ext2 文件操作表
 */
struct file_ops ext2_file_ops = {
	.open = ext2_file_open,
	.release = ext2_file_release,
	.read = ext2_file_read,
	.write = ext2_file_write,
};
