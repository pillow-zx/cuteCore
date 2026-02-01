#include <stdint.h>

#include "kernel/common.h"
#include "log.h"

static const uint64_t STDOUT_FD = 1;

int64_t sys_write(const uint64_t fd, const char *buf, uint64_t len) {
    int64_t ret = -1;
    switch (fd) {
        case STDOUT_FD: {
            uart_write(buf, len);
            ret = (int64_t)len;
            break;
        }
        default: {
            panic("Unsupported fd %lu in sys_write", fd);
            ret = -1;
            shutdown();
        }
    }
    return ret;
}
