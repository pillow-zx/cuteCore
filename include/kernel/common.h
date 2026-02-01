#ifndef __KERNEL_COMMON_H__
#define __KERNEL_COMMON_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "generated/autoconf.h"

// Include all module headers
#include "arch/riscv/trap.h"
#include "driver/uart.h"
#include "kernel/console.h"
#include "kernel/loader.h"
#include "kernel/shutdown.h"
#include "syscall/syscall.h"
#include "task/manager.h"
#include "utils/klib.h"
#include "utils/utils.h"

#endif // __KERNEL_COMMON_H__