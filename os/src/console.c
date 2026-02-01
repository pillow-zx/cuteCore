#include <stdint.h>

#include "kernel/common.h"

static inline int32_t printstr(const char *s) {
    if (!s) { return 0; }
    int len = 0;
    const char *p = s;
    while (*p++) { len++; }
    uart_write(s, len);
    return len;
}

static int32_t printnum(uint64_t val, int32_t base, int32_t is_signed, int32_t neg) {
    char buf[32];
    int32_t i = 0, cnt = 0;

    if (is_signed && neg) { val = -val; }

    do {
        int digit = val % base;
        buf[i++] = (digit < 10) ? '0' + digit : 'a' + digit - 10;
        val /= base;
    } while (val);

    if (neg) { buf[i++] = '-'; }

    while (--i >= 0) {
        uart_write(&buf[i], 1);
        cnt++;
    }

    return cnt;
}

static int32_t vprintf(const char *fmt, va_list ap) {
    int32_t cnt = 0;
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            uart_write(fmt, 1);
            cnt++;
            continue;
        }

        fmt++;
        if (*fmt == '\0') {
            uart_write("%", 1);
            break;
        }

        int32_t longflag = 0, longlongflag = 0;
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
                uint64_t val;
                int32_t neg = 0;
                if (longlongflag) {
                    val = va_arg(ap, long long);
                } else if (longflag) {
                    val = va_arg(ap, int64_t);
                } else {
                    val = va_arg(ap, int32_t);
                }
                if (val < 0) { neg = 1; }
                cnt += printnum(val, 10, 1, neg);
                break;
            }
            case 'u': {
                uint64_t val;
                if (longlongflag) {
                    val = va_arg(ap, uint64_t);
                } else if (longflag) {
                    val = va_arg(ap, uint64_t);
                } else {
                    val = va_arg(ap, uint32_t);
                }
                cnt += printnum(val, 10, 0, 0);
                break;
            }
            case 'x': {
                uint64_t val;
                if (longlongflag) {
                    val = va_arg(ap, uint64_t);
                } else if (longflag) {
                    val = va_arg(ap, uint64_t);
                } else {
                    val = va_arg(ap, uint32_t);
                }
                cnt += printnum(val, 16, 0, 0);
                break;
            }
            case 'p': {
                void *ptr = va_arg(ap, void *);
                cnt += printstr("0x");
                cnt += printnum((uint64_t)ptr, 16, 0, 0);
                break;
            }
            case 'c': {
                char c = (char)va_arg(ap, int32_t);
                uart_write(&c, 1);
                cnt++;
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                cnt += printstr(s);
                break;
            }
            case '%': {
                uart_write("%", 1);
                cnt++;
                break;
            }
            default: {
                uart_write("%", 1);
                uart_write(fmt, 1);
                cnt += 2;
                break;
            }
        }
    }
    return cnt;
}

int32_t printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int32_t cnt = vprintf(fmt, ap);
    va_end(ap);
    return cnt;
}
