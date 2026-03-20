#include "mm/kalloc.h"
#include "utils.h"

#define NUM_CACHES 9
#define MAX_CACHE_SIZE 2048

#define LARGE_ALLOC_MAGIC 0xDEADBEEFCAFEBABEULL

static struct cache *caches[NUM_CACHES];
static const u32 sizes[NUM_CACHES] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

void kmallocinit(void)
{
	for (i32 i = 0; i < NUM_CACHES; i++) {
		caches[i] = cache_create(sizes[i]);
	}
}

void *kmalloc(usize size)
{
	if (size == 0) {
		return nullptr;
	}

	if (size > MAX_CACHE_SIZE) {
		void *page = kalloc();
		if (page == nullptr) {
			return nullptr;
		}
		*(u64 *)page = LARGE_ALLOC_MAGIC;
		return (char *)page + sizeof(u64);
	}

	for (i32 i = 0; i < NUM_CACHES; i++) {
		if (size <= sizes[i]) {
			return cache_alloc(caches[i]);
		}
	}

	return nullptr;
}

void kmfree(void *ptr)
{
	if (ptr == nullptr) {
		return;
	}

	void *page = (void *)PGROUNDDOWN((u64)ptr);
	if (*(u64 *)page == LARGE_ALLOC_MAGIC) {
		kfree(page);
		return;
	}

	struct slab *s = (struct slab *)page;
	cache_free(s->cache, ptr);
}

void cache_shrink_all(void)
{
	for (i32 i = 0; i < NUM_CACHES; i++) {
		if (caches[i] != nullptr) {
			cache_shrink(caches[i]);
		}
	}
}
