#include "kernel/sbi.h"

#define SBI_EXT_SRST 0x53525354
#define SBI_SRST_SHUTDOWN 0x00000000
#define SBI_SRST_COLD_REBOOT 0x00000001

void shutdown(void)
{
	register u64 a0 asm("a0") = 0;
	register u64 a1 asm("a1") = 0;
	register u64 a7 asm("a7") = SBI_EXT_SRST;
	register u64 a6 asm("a6") = 0;
	asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a6), "r"(a7) : "memory");
	while (true)
		;
}
