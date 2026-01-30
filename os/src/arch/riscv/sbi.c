#include "defs.h"
#include <stdint.h>

inline void shutdown() { *(volatile uint32_t *)(SIFIVE_TEST_ADDR) = 0x5555; }
