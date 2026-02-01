#ifndef __KERNEL_CONSOLE_H__
#define __KERNEL_CONSOLE_H__

#include "utils/utils.h"
#include <stdarg.h>
#include <stdint.h>

// console.c
__format(printf, 1, 2) int32_t printf(const char *fmt, ...);

#endif // __KERNEL_CONSOLE_H__