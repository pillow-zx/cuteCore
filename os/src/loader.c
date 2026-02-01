#include <stdint.h>

#include "defs.h"
#include "log.h"
#include "task.h"
#include "klib.h"



void load_apps() {
    extern uint8_t _num_app[];
    size_t num_app = get_num_app();

    uintptr_t *app_start = (uintptr_t *)(_num_app + sizeof(uintptr_t));

    for (size_t i = 0; i < num_app; i++) {
        uintptr_t base_i = get_base_i(i);

        memset((void *)base_i, 0, APP_SIZE_LIMIT);

        uintptr_t src_addr = app_start[i];
        size_t src_len = app_start[i + 1] - app_start[i];

        memcpy((void *)base_i, (void *)src_addr, src_len);
    }

    asm volatile("fence.i");
}

