#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include "generated/autoconf.h"

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_INFO 4

#ifdef CONFIG_LOG_INFO
#define LOG_LEVEL_MAX LOG_LEVEL_INFO
#elif defined(CONFIG_LOG_DEBUG)
#define LOG_LEVEL_MAX LOG_LEVEL_DEBUG
#elif defined(CONFIG_LOG_WARN)
#define LOG_LEVEL_MAX LOG_LEVEL_WARN
#elif defined(CONFIG_LOG_ERROR)
#define LOG_LEVEL_MAX LOG_LEVEL_ERROR
#else
#define LOG_LEVEL_MAX LOG_LEVEL_NONE
#endif

#define static_assert _Static_assert

#define __always_inline static inline __attribute__((__always_inline__))
#define __inline static inline
#define __noreturn __attribute__((__noreturn__))
#define __nonull(...) __attribute__((nonnull(__VA_ARGS__)))
#define __nothrow __attribute__((__nothrow__))
#define __format(type, fmt_idx, arg_idx) __attribute__((format(type, fmt_idx, arg_idx)))
#define __access(mode, ref_idx, size_idx) __attribute__((access(mode, ref_idx, size_idx)))
#define __maybe_unused __attribute__((__unused__))
#define __aligned(x) __attribute__((aligned(x)))
#define __packed __attribute__((__packed__))
#define __malloc __attribute__((__malloc__))
#define __alloc_size(x) __attribute__((alloc_size(x)))
#define __must_check __attribute__((__warn_unused_result__))
#define __aligned_ret(x) __attribute__(assume_aligned(x))
#define __hot __attribute__((hot))

// tlsf.c
void tlsf_init(void *mem, size_t size);
__alloc_size(1) __malloc void *kmalloc(size_t size);
__alloc_size(2) __must_check void *krealloc(void *ptr, size_t size);
__nothrow void kfree(void *ptr);

#endif // _UTILS_H_
