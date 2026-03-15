#include "config.h"
#include "types.h"
#include "log.h"

#define UART_BASE UART0
#define UART_REG(reg) ((volatile u8 *)(UART_BASE + reg))

#define DLL 0
#define RBR 0
#define RHR 0
#define THR 0
#define IER 1
#define DLM 1
#define FCR 2
#define LCR 3
#define LSR 5
#define IER_RX_ENABLE (1 << 0)
#define IER_TX_ENABLE (1 << 1)
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_FIFO_CLEAR (3 << 1)
#define FCR_FIFO_RESET_RX (1 << 1)
#define FCR_FIFO_RESET_TX (1 << 2)
#define LCR_EIGHT_BITS (3 << 0)
#define LCR_BAUD_LATCH (1 << 7)
#define LSR_TX_IDLE (1 << 5)
#define LSR_RX_READY (1 << 0)
#define WRITE_REG(reg, v) (*UART_REG(reg) = v)

void uart_putc(char c)
{
	while ((*UART_REG(LSR) & LSR_TX_IDLE) == 0)
		;
	*UART_REG(THR) = c;
}

i32 uart_getc(void)
{
	while ((*UART_REG(LSR) & LSR_RX_READY) == 0)
		;
	return *UART_REG(RBR);
}

void uartinit(void)
{
	WRITE_REG(IER, 0x00);

	WRITE_REG(LCR, LCR_BAUD_LATCH);

	WRITE_REG(DLL, 0x03);
	WRITE_REG(DLM, 0x00);

	WRITE_REG(LCR, LCR_EIGHT_BITS);

	WRITE_REG(FCR, FCR_FIFO_RESET_RX | FCR_FIFO_RESET_TX);
	WRITE_REG(FCR, FCR_FIFO_ENABLE);
	while (*UART_REG(LSR) & LSR_RX_READY)
		(void)*UART_REG(RBR);

	Log("Uart init success");
}
