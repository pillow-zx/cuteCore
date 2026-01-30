#include "defs.h"

void sys_exit(int64_t code) {
    printf("Process exited with code %ld\n", code);
    shutdown();
    while (1);
}
