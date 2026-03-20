#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "config.h"

#define BITS_U8(n) ((u8)(1) << (n))
#define BITS_U64(n) ((u64)(1) << (n))
#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#define MAP(func, ...) func(__VA_ARGS__)

#define static_assert _Static_assert
#define __always_inline static inline __attribute__((always_inline))
#define __inline static inline
#define __noinline __attribute__((noinline))
#define __noreturn __attribute__((noreturn))
#define __aligned(x) __attribute__((aligned(x)))
#define __pure __attribute__((pure))
#define __format(type, fmt_idx, arg_idx) \
	__attribute__((format(type, fmt_idx, arg_idx)))
#define __access(mode, ref_idx, size_idx) \
	__attribute__((access(mode, ref_idx, size_idx)))
#define __maybe_unused __attribute__((unused))
#define __packed __attribute__((packed))
#define __section(x) __attribute__(("x"))

#define ALIGNUP(size, align) (((size) + (align) - 1) & ~((align) - 1))
#define ALIGNDOWN(size, align) ((size) & ~((align) - 1))

struct slab {
	struct slab *next;
	struct slab *prev;
	void *list;
	int freenum;
	int objectsnum;
	struct cache *cache;
};

struct cache {
	uint32_t objsz;
	struct slab *slabs_full;
	struct slab *slabs_partial;
	struct slab *slabs_empty;
	int empty_slabs;
};

__access(write_only, 1, 3) __access(read_only, 2, 3)
void *memcpy(void *s1, const void *s2, usize n);
void *memmove(void *s1, const void *s2, usize n);
__access(write_only, 1, 3) void *memset(void *s, i32 c, size_t n);
int memcmp(const void *s1, const void *s2, usize n);
char *strcpy(char *s1, const char *s2);
char *strcat(char *s1, const char *s2);
int strcmp(const char *s1, const char *s2);
__pure size_t strlen(const char *s);

struct cache *cache_create(u32 size);
void *cache_alloc(struct cache *cache);
void cache_free(struct cache *cache, void *obj);
void cache_shrink(struct cache *cache);
void cache_shrink_all(void);

void kmallocinit(void);
void *kmalloc(usize size);
void kmfree(void *ptr);

#endif
