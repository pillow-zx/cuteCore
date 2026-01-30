#include <stdint.h>
#include "defs.h"



inline void shutdown() {
    *(volatile uint32_t *)(SIFIVE_TEST_ADDR) = 0x5555;
}
