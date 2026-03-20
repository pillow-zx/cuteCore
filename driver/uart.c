/**
 * @file uart.c
 * @brief 16550 兼容 UART 驱动
 *
 * 本文件实现了与 QEMU virt 机器兼容的 UART 驱动，
 * 提供基本的串口输入输出功能，用于内核调试输出和用户交互。
 *
 * 硬件特性：
 * - 兼容 NS16550A 标准
 * - 支持 8 位数据位、无校验、1 位停止位（8N1）
 * - 使用轮询方式收发（非中断驱动）
 */

#include "config.h"
#include "fs.h"
#include "log.h"
#include "types.h"

/* ============================================================================
 * UART Base Address
 * ============================================================================ */

/** @brief UART 设备基地址（QEMU virt 机器） */
#define UART_BASE UART0

/**
 * @brief 获取 UART 寄存器指针
 *
 * @param reg 寄存器偏移量
 *
 * @return 指向该寄存器的 volatile 指针
 */
#define UART_REG(reg) ((volatile u8 *)(UART_BASE + reg))

/* ============================================================================
 * UART Register Offsets (16550 compatible)
 * ============================================================================ */

/*
 * 寄存器别名说明：
 * 部分 16550 寄存器共享同一地址，具体功能由 LCR.DLAB 位决定：
 * - DLAB = 0: RBR/THR/IER 正常功能
 * - DLAB = 1: DLL/DLM 用于波特率设置
 */

/** @brief Divisor Latch Low - 波特率除数低字节（DLAB=1） */
#define DLL 0

/** @brief Receiver Buffer Register - 接收缓冲区（DLAB=0，只读） */
#define RBR 0

/** @brief Receive Holding Register - 接收保持寄存器（RBR 的别名） */
#define RHR 0

/** @brief Transmit Holding Register - 发送保持寄存器（DLAB=0，只写） */
#define THR 0

/** @brief Interrupt Enable Register - 中断使能寄存器（DLAB=0） */
#define IER 1

/** @brief Divisor Latch High - 波特率除数高字节（DLAB=1） */
#define DLM 1

/** @brief FIFO Control Register - FIFO 控制寄存器（只写） */
#define FCR 2

/** @brief Line Control Register - 线路控制寄存器 */
#define LCR 3

/** @brief Line Status Register - 线路状态寄存器（只读） */
#define LSR 5

/* ============================================================================
 * IER (Interrupt Enable Register) Bits
 * ============================================================================ */

/** @brief 接收数据可用中断使能 */
#define IER_RX_ENABLE (1 << 0)

/** @brief 发送保持寄存器空中断使能 */
#define IER_TX_ENABLE (1 << 1)

/* ============================================================================
 * FCR (FIFO Control Register) Bits
 * ============================================================================ */

/** @brief 启用 FIFO */
#define FCR_FIFO_ENABLE (1 << 0)

/** @brief 清空 FIFO（用于一次性清除） */
#define FCR_FIFO_CLEAR (3 << 1)

/** @brief 复位接收 FIFO */
#define FCR_FIFO_RESET_RX (1 << 1)

/** @brief 复位发送 FIFO */
#define FCR_FIFO_RESET_TX (1 << 2)

/* ============================================================================
 * LCR (Line Control Register) Bits
 * ============================================================================ */

/** @brief 8 位数据位 */
#define LCR_EIGHT_BITS (3 << 0)

/** @brief 波特率除数访问使能（DLAB） */
#define LCR_BAUD_LATCH (1 << 7)

/* ============================================================================
 * LSR (Line Status Register) Bits
 * ============================================================================ */

/** @brief 发送保持寄存器空（可写入下一个字符） */
#define LSR_TX_IDLE (1 << 5)

/** @brief 接收数据就绪（可读取一个字符） */
#define LSR_RX_READY (1 << 0)

/* ============================================================================
 * Register Access Macro
 * ============================================================================ */

/**
 * @brief 写入 UART 寄存器
 *
 * @param reg 寄存器偏移量
 * @param v   要写入的值
 */
#define WRITE_REG(reg, v) (*UART_REG(reg) = v)

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * @brief 输出单个字符到 UART
 *
 * 等待发送保持寄存器空闲后写入字符。此函数为阻塞调用，
 * 直到字符成功发送至硬件缓冲区。
 *
 * @param c 要输出的字符
 *
 * @note 用于内核日志输出和 printf 等函数的底层实现
 */
void uart_putc(const char c)
{
	while ((*UART_REG(LSR) & LSR_TX_IDLE) == 0)
		;
	*UART_REG(THR) = c;
}

/**
 * @brief 从 UART 读取单个字符
 *
 * 等待接收缓冲区有数据后读取。此函数为阻塞调用，
 * 直到接收到一个字符。
 *
 * @return 读取到的字符（0-255）
 *
 * @note 当前实现为轮询模式，CPU 会忙等待数据到达
 */
i32 uart_getc(void)
{
	while ((*UART_REG(LSR) & LSR_RX_READY) == 0)
		;
	return *UART_REG(RBR);
}

/**
 * @brief 初始化 UART 设备
 *
 * 配置 UART 为以下参数：
 * - 波特率：38400 bps（QEMU 虚拟串口，实际值不重要）
 * - 数据位：8 位
 * - 校验位：无
 * - 停止位：1 位
 * - FIFO：启用并清空
 *
 * 初始化步骤：
 * 1. 禁用所有中断
 * 2. 设置波特率除数（DLAB=1）
 * 3. 设置 8N1 模式（DLAB=0）
 * 4. 启用并清空 FIFO
 * 5. 清空接收缓冲区中的残留数据
 *
 * @note 在内核启动早期调用，是 console 初始化的基础
 */
void uartinit(void)
{
	/* 禁用所有中断 */
	WRITE_REG(IER, 0x00);

	/* 设置 DLAB=1 以访问波特率除数寄存器 */
	WRITE_REG(LCR, LCR_BAUD_LATCH);

	/* 设置波特率除数（38400 bps） */
	WRITE_REG(DLL, 0x03);
	WRITE_REG(DLM, 0x00);

	/* 设置 8N1 模式，同时清除 DLAB */
	WRITE_REG(LCR, LCR_EIGHT_BITS);

	/* 启用 FIFO 并清空缓冲区 */
	WRITE_REG(FCR, FCR_FIFO_RESET_RX | FCR_FIFO_RESET_TX);
	WRITE_REG(FCR, FCR_FIFO_ENABLE);

	/* 清空接收缓冲区中可能存在的残留数据 */
	while (*UART_REG(LSR) & LSR_RX_READY)
		(void)*UART_REG(RBR);

	Log("Uart init success");
}

/**
 * @brief 从 UART 设备读取数据
 *
 * 非阻塞读取，只返回当前可用的数据。
 *
 * @param f      文件对象（未使用）
 * @param buf    输出缓冲区
 * @param len    请求读取的字节数
 * @param offset 偏移量指针（未使用）
 *
 * @return 实际读取的字节数
 */
i32 uart_read(const struct file *f, void *buf, usize len, off_t *offset)
{
	(void)f;
	(void)offset;

	char *p = (char *)buf;
	usize n = 0;

	while (n < len) {
		if ((*UART_REG(LSR) & LSR_RX_READY) == 0)
			break;
		p[n++] = (char)*UART_REG(RBR);
	}

	return (i32)n;
}

/**
 * @brief 向 UART 设备写入数据
 *
 * @param f      文件对象（未使用）
 * @param buf    输入缓冲区
 * @param len    请求写入的字节数
 * @param offset 偏移量指针（未使用）
 *
 * @return 实际写入的字节数
 */
i32 uart_write(struct file *f, const void *buf, usize len, off_t *offset)
{
	(void)f;
	(void)offset;

	const char *p = (const char *)buf;

	for (usize i = 0; i < len; i++) {
		/* 等待发送缓冲区空闲 */
		while ((*UART_REG(LSR) & LSR_TX_IDLE) == 0)
			;
		*UART_REG(THR) = p[i];
	}

	return (i32)len;
}

/**
 * @brief UART 设备文件操作表
 */
struct file_ops uart_ops = {
	.open = nullptr,
	.release = nullptr,
	.read = uart_read,
	.write = uart_write,
};
