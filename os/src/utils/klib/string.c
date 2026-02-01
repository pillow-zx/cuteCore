#include "klib.h"


void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)(dest);
    const char *s = (const char *)(src);
    for (size_t i = 0; i < n; i++) { d[i] = s[i]; }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *a = (unsigned char *)(s);
    for (size_t i = 0; i < n; i++) { a[i] = (unsigned char)c; }
    return s;
}

int32_t memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = (const unsigned char *)(s1);
    const unsigned char *b = (const unsigned char *)(s2);
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) { return (int32_t)(a[i] - b[i]); }
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') { len++; }
    return len;
}

int32_t strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}
