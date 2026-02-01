#include <stddef.h>

#include "lib.h"

extern char start_bss[];
extern char end_bss[];

void clear_bss() {
    for (char *p = start_bss; p < end_bss; p++) { *p = 0; }
}

__attribute__((section(".text.entry"))) void _start() {
    clear_bss();
    exit(main());
}

char *strcpy(char *s, const char *t) {
    char *os;

    os = s;
    while ((*s++ = *t++) != 0);
    return os;
}

int strcmp(const char *p, const char *q) {
    while (*p && *p == *q) { p++, q++; }
    return (unsigned char)*p - (unsigned char)*q;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') { len++; }
    return len;
}

void *memset(void *dst, int c, size_t n) {
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++) { cdst[i] = c; }
    return dst;
}

char *strchr(const char *s, char c) {
    for (; *s; s++) {
        if (*s == c) { return (char *)s; }
    }
    return 0;
}

int atoi(const char *s) {
    int n;

    n = 0;
    while ('0' <= *s && *s <= '9') { n = n * 10 + *s++ - '0'; }
    return n;
}

void *memmove(void *vdst, const void *vsrc, int n) {
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    if (src > dst) {
        while (n-- > 0) { *dst++ = *src++; }
    } else {
        dst += n;
        src += n;
        while (n-- > 0) { *--dst = *--src; }
    }
    return vdst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const char *p1 = s1, *p2 = s2;
    while (n-- > 0) {
        if (*p1 != *p2) { return *p1 - *p2; }
        p1++;
        p2++;
    }
    return 0;
}

void *memcpy(void *dst, const void *src, size_t n) { return memmove(dst, src, n); }
