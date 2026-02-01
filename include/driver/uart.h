#ifndef __DRIVER_UART_H__
#define __DRIVER_UART_H__

// uart.c
void uart_init();
void uart_write(const char *buf, int len);
void uart_read(char *buf, int limit);

#endif // __DRIVER_UART_H__