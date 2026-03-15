#include <stdarg.h>

#include "kernel.h"
#include "driver.h"

static usize printstr(const char *s)
{
	if (!s)
		return 0;
	const usize len = strlen(s);
	while (*s) {
		uart_putc(*s++);
	}

	return len;
}

static usize printnum(unsigned long val, int base, bool is_signed, bool neg)
{
	char buf[32];
	i32 i = 0, cnt = 0;

	if (is_signed && neg)
		val = -val;
	do {
		i32 digit = val % base;
		buf[i++] = (digit < 10) ? '0' + digit : 'a' + digit - 10;
		val /= base;
	} while (val);

	if (neg)
		buf[i++] = '-';

	while (--i >= 0) {
		uart_putc(buf[i]);
		cnt++;
	}

	return cnt;
}

static i32 vprintf(const char *fmt, va_list ap)
{
	i32 cnt = 0;
	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			uart_putc(*fmt);
			cnt++;
			continue;
		}

		fmt++;
		if (*fmt == '\0') {
			uart_putc('%');
			break;
		}

		i32 longflag = 0, longlongflag = 0;
		if (*fmt == 'l') {
			fmt++;
			if (*fmt == 'l') {
				longlongflag = 1;
				fmt++;
			} else {
				longflag = 1;
			}
		}

		switch (*fmt) {
		case 'd': {
			i64 sval = 0;
			i32 neg = 0;
			if (longlongflag) {
				sval = va_arg(ap, long long);
			} else if (longflag) {
				sval = va_arg(ap, i64);
			} else {
				sval = va_arg(ap, i32);
			}
			if (sval < 0) {
				neg = 1;
			}
			cnt += printnum((u64)sval, 10, 1, neg);
			break;
		}
		case 'u': {
			u64 val;
			if (longlongflag) {
				val = va_arg(ap, u64);
			} else if (longflag) {
				val = va_arg(ap, u64);
			} else {
				val = va_arg(ap, u32);
			}
			cnt += printnum(val, 10, 0, 0);
			break;
		}
		case 'x': {
			u64 val;
			if (longlongflag) {
				val = va_arg(ap, u64);
			} else if (longflag) {
				val = va_arg(ap, u64);
			} else {
				val = va_arg(ap, u32);
			}
			cnt += printnum(val, 16, 0, 0);
			break;
		}
		case 'p': {
			void *ptr = va_arg(ap, void *);
			cnt += printstr("0x");
			cnt += printnum((u64)ptr, 16, 0, 0);
			break;
		}
		case 'c': {
			char c = (char)va_arg(ap, i32);
			uart_putc(c);
			cnt++;
			break;
		}
		case 's': {
			const char *s = va_arg(ap, const char *);
			cnt += printstr(s);
			break;
		}
		case '%': {
			uart_putc('%');
			cnt++;
			break;
		}
		default: {
			uart_putc('%');
			uart_putc(*fmt);
			cnt += 2;
			break;
		}
		}
	}
	return cnt;
}

i32 printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	i32 cnt = vprintf(fmt, ap);
	va_end(ap);
	return cnt;
}
