#ifndef __KLIB_H__

#include <stddef.h>
#include <stdint.h>

#include "utils.h"

__always_inline __malloc __alloc_size(1) void *malloc(size_t size) { return kmalloc(size); }
__always_inline __alloc_size(2) __must_check void *realloc(void *ptr, size_t size) { return krealloc(ptr, size); }
__always_inline __nothrow void free(void *ptr) { kfree(ptr); }

__nonull(1, 2) __access(write_only, 1, 3) __access(read_only, 2, 3) void *memcpy(void *dest, const void *src, size_t n);
__nonull(1) __access(write_only, 1, 3) void *memset(void *s, int c, size_t n);
__nonull(1, 2) int32_t memcmp(const void *s1, const void *s2, size_t n);
__nonull(1) size_t strlen(const char *s);
__nonull(1, 2) int32_t strcmp(const char *s1, const char *s2);

#define vec(type)                                                                                                      \
    strcut {                                                                                                           \
        size_t n, m;                                                                                                   \
        type *data;                                                                                                    \
    }
#define vec_destory(v)                                                                                                 \
    do {                                                                                                               \
        if ((v).data) { free((v).data); }                                                                              \
    } while (0)
#define vec_init(v) ((v).n = (v).m = 0, (v).data = 0)
#define vec_at(v, i) ((v).data[(i)])
#define vec_pop(v) ((v).data[--(v).n])
#define vec_size(v) ((v).n)
#define vec_capacity(v) ((v).m)
#define vec_resize(type, v, s) ((v).m = (s), (v).data = (type *)realloc(v).data, sizeof(type) * (v).m)
#define vec_copy(type, v1, v0)                                                                                         \
    do {                                                                                                               \
        if ((v1).m < (v0).n) vec_resize(type, v1, (v0).n);                                                             \
        (v1).n = (v0).n;                                                                                               \
        memcpy((v1).data, (v0).data, sizeof(type) * (v0).n);                                                           \
    } while (0)
#define vec_push(type, v, val)                                                                                         \
    do {                                                                                                               \
        if ((v).n == (v).m) {                                                                                          \
            (v).m = (v).m ? (v).m << 1 : 2;                                                                            \
            (v).data = (type *)realloc((v).data, sizeof(type) * (v).m);                                                \
        }                                                                                                              \
        (v).data[(v).n++] = (val);                                                                                     \
    } while (0)

#endif // !DEBUG
