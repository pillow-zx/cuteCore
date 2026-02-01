#ifndef __LIB_H__
#define __LIB_H__


#include <stddef.h>
#include <stdint.h>


#define __always_inline static inline __attribute__((__always_inline__))
#define __inline static inline
#define __noreturn __attribute__((__noreturn__))
#define __nonull(...) __attribute__((nonnull(__VA_ARGS__)))
#define __format(type, fmt_idx, arg_idx) __attribute__((format(type, fmt_idx, arg_idx)))
#define __access(mode, ref_idx, size_idx) __attribute__((access(mode, ref_idx, size_idx)))
#define __maybe_unused __attribute__((__unused__))
#define __aligned(x) __attribute__((aligned(x)))
#define __packed __attribute__((__packed__))
#define __alloc_size(x) __attribute__((alloc_size(x)))
#define __must_check __attribute__((__warn_unused_result__))
#define __pure __attribute__((__pure__))



// printf.c
__format(printf, 2, 0) void vprintf(int fd, const char *fmt, __builtin_va_list ap);
__format(printf, 2, 3) void fprintf(int fd, const char *fmt, ...);
__format(printf, 1, 2) void printf(const char *fmt, ...);

// lib.c
char *gets(char *buf, int max);
__nonull(1, 2) char *strcpy(char *s, const char *t);
__pure __nonull(1) int strcmp(const char *p, const char *q);
__pure __nonull(1) size_t strlen(const char *s);
__nonull(1) void *memset(void *dst, int c, size_t n);
__pure __nonull(1) char *strchr(const char *s, char c);
__pure __must_check __nonull(1) int atoi(const char *s);
__nonull(1, 2) void *memmove(void *vdst, const void *vsrc, int n);
__pure __nonull(1, 2) int memcmp(const void *s1, const void *s2, size_t n);
__nonull(1 ,2) void *memcpy(void *vdst, const void *vsrc, size_t n);


#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#define SYSCALL_YIELD 124
#define SYSCALL_GET_TIME 169


__always_inline intptr_t syscall(intptr_t id, intptr_t arg_a0, intptr_t arg_a1, intptr_t arg_a2) {
    register uintptr_t a0 asm("a0") = arg_a0;
    register uintptr_t a1 asm("a1") = arg_a1;
    register uintptr_t a2 asm("a2") = arg_a2;
    register uintptr_t a7 asm("a7") = id;

    asm volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a7)
        : "memory"
    );

    return (intptr_t)a0;
}


__always_inline intptr_t write(const size_t fd, const void *buffer, size_t len) {
    return syscall(SYSCALL_WRITE, (intptr_t)fd, (intptr_t)buffer, (intptr_t)len);
}

__always_inline __noreturn void exit(const intptr_t exit_code) {
    syscall(SYSCALL_EXIT, (intptr_t)exit_code, 0, 0);
    while (1);
}

__always_inline intptr_t yield() {
    return syscall(SYSCALL_YIELD, 0, 0, 0);
}

__always_inline intptr_t time() {
    return syscall(SYSCALL_GET_TIME, 0, 0, 0);
}


void _start();
int main();




#endif // LIB_H
