#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "types.h"
#include "utils.h"

__format(printf, 1, 2) i32 printf(const char *fmt, ...);

#endif
