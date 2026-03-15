#include "utils.h"

void *memcpy(void *s1, const void *s2, usize n)
{
	unsigned char *dst = s1;
	const unsigned char *src = s2;
	for (usize i = 0; i < n; i++) {
		dst[i] = src[i];
	}

	return s1;
}

int memcmp(const void *s1, const void *s2, usize n)
{
	const unsigned char *p1 = s1;
	const unsigned char *p2 = s2;

	for (usize i = 0; i < n; i++) {
		if (p1[i] != p2[i])
			return p1[i] - p2[i];
	}

	return 0;
}

void *memset(void *s, int c, usize n)
{
	unsigned char *p = s;

	for (usize i = 0; i < n; i++)
		p[i] = (unsigned char)c;

	return 0;
}

void *memmove(void *dest, const void *src, usize n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	if (d == s)
		return dest;

	if (d < s) {
		for (usize i = 0; i < n; i++)
			d[i] = s[i];
	} else {
		for (usize i = n; i > 0; i--)
			d[i - 1] = s[i - 1];
	}

	return dest;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}

	return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dest, const char *src)
{
	char *ret = dest;

	while ((*dest++ = *src++) != '\0')
		;

	return ret;
}

char *strcat(char *s1, const char *s2)
{
	char *ret = s1;

	while (*s1)
		s1++;
	while ((*s1++ = *s2++) != '\0')
		;

	return ret;
}

size_t strlen(const char *s)
{
	const char *p = s;

	while (*p) {
		p++;
	}

	return p - s;
}
