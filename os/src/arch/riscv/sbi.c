#include <stdint.h>

#include "riscv.h"
#include "defs.h"

inline void shutdown() { *(volatile uint32_t *)(SIFIVE_TEST_ADDR) = 0x5555; }
