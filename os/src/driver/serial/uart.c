#include <stdint.h>

#include "riscv.h"
#include "utils.h"

__always_inline void uart_putc(char c) {
    while ((*UART_REG(LSR) & LSR_TX_IDLE) == 0);
    *UART_REG(THR) = c;
}

__always_inline void uart_puts(const char *s) {
    while (*s) { uart_putc(*s++); }
}

__always_inline int32_t uart_getc() {
    while ((*UART_REG(LSR) & LSR_RX_READY) == 0);
    return *UART_REG(RBR);
}

void uart_init() {
    *UART_REG(1) = 0x00;
    *UART_REG(3) = 0x03;
}

void uart_write(const char *buf, int32_t len) {
    while (len--) { uart_putc(*buf++); }
}

void uart_read(char *buf, int32_t limit) {
    int i = 0;
    while (i < limit - 1) {
        char c = uart_getc();

        if (c == '\r' || c == '\n') {
            buf[i] = '\0';
            uart_putc('\n');
            break;
        }

        if (c == '\b' || c == 127) {
            if (i > 0) {
                if (i > 0) {
                    i--;
                    uart_puts("\b \b");
                }
            }
            continue;
        }

        if (c >= 32 && c <= 126) {
            buf[i++] = c;
            uart_putc(c);
        }
    }
}
