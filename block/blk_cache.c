#include "driver.h"
#include "log.h"
#include "fs.h"

struct blk_cache blk_caches[NBLKNUM];

struct blk_cache *bread(u64 no)
{
	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].valid && blk_caches[i].blk_no == no) {
			blk_caches[i].refcnt++;
			return &blk_caches[i];
		}
	}

	struct blk_cache *bcache = nullptr;

	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].refcnt == 0) {
			bcache = &blk_caches[i];
			break;
		}
	}

	if (!bcache)
		panic("no free block cache");

	if (bcache->valid && bcache->dirty) {
		blkwrite(bcache->blk_no, bcache->data);
	}

	blkread(no, bcache->data);

	bcache->blk_no = no;
	bcache->valid = true;
	bcache->dirty = false;
	bcache->refcnt = 1;

	return bcache;
}

void bwrite(struct blk_cache *blk)
{
	if (!blk || !blk->valid)
		return;
	blk->dirty = true;
}

void brelse(struct blk_cache *blk)
{
	if (!blk)
		return;
	if (blk->refcnt <= 0)
		panic("brelse");

	blk->refcnt--;
}

void bsync(void)
{
	for (i32 i = 0; i < NBLKNUM; i++) {
		if (blk_caches[i].valid && blk_caches[i].dirty) {
			blkwrite(blk_caches[i].blk_no, blk_caches[i].data);
			blk_caches[i].dirty = false;
		}
	}
}
