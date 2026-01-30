#include <stddef.h>


#include "defs.h"
#include "log.h"

void clear_bss() {
    extern unsigned long sbss, ebss;
    unsigned long start = sbss;
    while (start < ebss) {
        *((unsigned char *)start) = 0;
        start++;
    }
}

static void kernel_struct() { extern unsigned long stext, etext, srodata, erodata, sdata, edata, sbss, ebss;
    Info("text: [%p, %p), size: %luKB", &stext, &etext, ((size_t)&etext - (size_t)&stext) / 1024);
    Info("rodata: [%p, %p), size: %luKB", &srodata, &erodata, ((size_t)&erodata - (size_t)&srodata) / 1024);
    Info("data: [%p, %p), size: %luKB", &sdata, &edata, ((size_t)&edata - (size_t)&sdata) / 1024);
    Info("bss: [%p, %p), size: %luKB", &sbss, &ebss, ((size_t)&ebss - (size_t)&sbss) / 1024);
    printf("\n");
}

void main() {
    clear_bss();
    uart_init();
    kernel_struct();
    shutdown();
}
