/**
 * @file blk_cache.c
 * @brief 块缓存层实现
 *
 * 本文件实现了简单的块缓存机制，用于缓存磁盘块数据，减少磁盘I/O操作。
 * 缓存采用固定大小的槽位数组，使用引用计数跟踪使用状态。
 */

#include "driver.h"
#include "log.h"
#include "fs.h"

/**
 * @brief 全局块缓存数组
 *
 * 固定大小为 NBLKNUM 个槽位，每个槽位缓存一个 BLKSZ 大小的磁盘块。
 */
struct blk_cache blk_caches[NBLKNUM];

/**
 * @brief 获取块数据
 *
 * 遵循"先查找，后替换"原则：
 * 1. 首先在缓存中查找目标块，若命中则增加引用计数并返回
 * 2. 若未命中，查找空闲槽位（refcnt == 0）
 * 3. 若槽位有脏数据，先写回磁盘
 * 4. 从磁盘读取目标块数据到缓存
 *
 * @param no 目标块号
 *
 * @return 成功返回块缓存指针；失败触发 panic
 *
 * @note 当前使用简单的首次空闲槽位算法，未来可替换为 LRU 算法
 * @note 缓存未命中时可能触发脏块写回
 */
struct blk_cache *bread(u64 no)
{
	// TODO: 这里使用了简单了查找第一个空闲槽位的算法，未来可替换为使用LRU算法
	// 缓存命中遍历
	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].valid && blk_caches[i].blk_no == no) {
			blk_caches[i].refcnt++;
			return &blk_caches[i];
		}
	}

	// 查找空闲槽位
	struct blk_cache *bcache = nullptr;
	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].refcnt == 0) {
			bcache = &blk_caches[i];
			break;
		}
	}

	if (!bcache)
		panic("no free block cache");

	// 写回脏数据
	if (bcache->valid && bcache->dirty) {
		blkwrite(bcache->blk_no, bcache->data);
	}

	// 读取新数据
	blkread(no, bcache->data);

	// 其他初始化
	bcache->blk_no = no;
	bcache->valid = true;
	bcache->dirty = false;
	bcache->refcnt = 1;

	return bcache;
}

/**
 * @brief 将缓存块标记为脏
 *
 * 标记后块将在下次被替换时写回磁盘，不会立即执行写操作。
 *
 * @param blk 待标记的块缓存指针
 *
 * @note 无效块或空指针将被忽略
 */
void bwrite(struct blk_cache *blk)
{
	if (!blk || !blk->valid)
		return;
	blk->dirty = true;
}

/**
 * @brief 释放块缓存引用
 *
 * 减少块缓存的引用计数，当引用计数归零时该槽位可被重新分配。
 *
 * @param blk 待释放的块缓存指针
 *
 * @note 引用计数为 0 时触发 panic，表示重复释放或逻辑错误
 */
void brelse(struct blk_cache *blk)
{
	if (!blk)
		return;
	if (blk->refcnt <= 0)
		panic("brelse");

	blk->refcnt--;
}

/**
 * @brief 同步所有脏块到磁盘
 *
 * 遍历所有缓存槽位，将标记为脏的块写入磁盘，并清除脏标志。
 * 通常在系统关机或定期同步时调用。
 */
void bsync(void)
{
	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].valid && blk_caches[i].dirty) {
			blkwrite(blk_caches[i].blk_no, blk_caches[i].data);
			blk_caches[i].dirty = false;
		}
	}
}
