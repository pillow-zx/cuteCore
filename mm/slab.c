#include "mm/kalloc.h"
#include "config.h"
#include "utils.h"

#define CACHEALIGN alignof(void *)

#define MAX_EMPTY_SLABS 2

static void list_remove(struct slab **head, struct slab *s)
{
	if (s->prev) {
		s->prev->next = s->next;
	} else {
		*head = s->next;
	}
	if (s->next) {
		s->next->prev = s->prev;
	}
}

static void list_insert(struct slab **head, struct slab *s)
{
	s->next = *head;
	s->prev = nullptr;
	if (*head) {
		(*head)->prev = s;
	}
	*head = s;
}

struct cache *cache_create(u32 size)
{
	struct cache *cache = (struct cache *)kalloc();
	if (cache == nullptr) {
		return nullptr;
	}

	cache->objsz = ALIGNUP(size, CACHEALIGN);
	cache->slabs_full = nullptr;
	cache->slabs_partial = nullptr;
	cache->slabs_empty = nullptr;
	cache->empty_slabs = 0;

	return cache;
}

static void slabinit(struct cache *cache, struct slab *s)
{
	char *p = (char *)s + sizeof(struct slab);

	s->list = p;
	s->objectsnum = (PGSIZE - sizeof(struct slab)) / cache->objsz;
	s->freenum = s->objectsnum;
	s->next = nullptr;
	s->prev = nullptr;
	s->cache = cache;

	for (i32 i = 0; i < s->objectsnum; i++) {
		char *next_obj = p + cache->objsz;
		*(void **)p = next_obj;
		p = next_obj;
	}
	*(void **)p = nullptr;
}

void *cache_alloc(struct cache *cache)
{
	struct slab *s = cache->slabs_partial;

	if (s == nullptr) {
		s = cache->slabs_empty;
		if (s == nullptr) {
			s = (struct slab *)kalloc();
			if (s == nullptr) {
				return 0;
			}
			slabinit(cache, s);
		} else {
			list_remove(&cache->slabs_empty, s);
			cache->empty_slabs--;
		}
		list_insert(&cache->slabs_partial, s);
	}

	void *obj = s->list;
	s->list = *(void **)obj;
	s->freenum--;

	if (s->freenum == 0) {
		list_remove(&cache->slabs_partial, s);
		list_insert(&cache->slabs_full, s);
	}

	return obj;
}

void cache_free(struct cache *cache, void *obj)
{
	struct slab *s = (struct slab *)PGROUNDDOWN((u64)obj);

	*(void **)obj = s->list;
	s->list = obj;
	s->freenum++;

	if (s->freenum == 1) {
		list_remove(&cache->slabs_full, s);
		list_insert(&cache->slabs_partial, s);
	} else if (s->freenum == s->objectsnum) {
		list_remove(&cache->slabs_partial, s);
		list_insert(&cache->slabs_empty, s);
		cache->empty_slabs++;

		if (cache->empty_slabs > MAX_EMPTY_SLABS) {
			cache_shrink(cache);
		}
	}
}

void cache_shrink(struct cache *cache)
{
	struct slab *s = cache->slabs_empty;
	struct slab *next;

	while (s != nullptr && cache->empty_slabs > MAX_EMPTY_SLABS) {
		next = s->next;
		list_remove(&cache->slabs_empty, s);
		cache->empty_slabs--;
		kfree((void *)s);
		s = next;
	}
}

