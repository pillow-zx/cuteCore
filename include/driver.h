#ifndef DRIVER_H
#define DRIVER_H

#include "types.h"

void driverinit(void);

void uart_putc(char c);
i32 uart_getc(void);
void uartinit(void);

void blkinit(void);
void blkread(u64 no, char *data);
void blkwrite(u64 no, const char *data);

#endif
