#include <stdint.h>

#include "arch/riscv/riscv.h"
#include "kernel/common.h"

inline void shutdown() { *(volatile uint32_t *)(SIFIVE_TEST_ADDR) = 0x5555; }
