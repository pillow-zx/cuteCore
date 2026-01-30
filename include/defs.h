#ifndef __DEFS_H_
#define __DEFS_H_


#include <stdarg.h>
#include <stdint.h>


#include "generated/autoconf.h"


// sbi.c
#ifdef CONFIG_ISA_RISCV

#define SIFIVE_TEST_ADDR 0x100000
#define UART0 0x10000000L

// UART device
#define RBR 0
#define THR 0
#define LSR 5
#define LSR_TX_IDLE (1 << 5)
#define LSR_RX_READY (1 << 0)
#define UART_REG(reg) ((volatile unsigned char *)(UART0 + reg))

#endif

void shutdown();




// uart.c
void uart_init();
void uart_write(const char *buf, int len);
void uart_read(char *buf, int limit);


// console.c
int32_t printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void panic(const char *s) __attribute__((noreturn));


#endif // __DEFS_H_
