#include "kernel/sbi.h"
#include "kernel/task.h"
#include "kernel/trap.h"
#include "mm/kalloc.h"
#include "mm/vm.h"
#include "fs.h"
#include "driver.h"
#include "utils.h"
#include "log.h"

void kernel_main(__maybe_unused i32 hartid)
{
	uartinit();
	kinit();
	kvminit();
	kvminithart();
	procinit();
	trapinit();
	blkinit();
	fileinit();
	fsinit();
	Log("Ready to run user processs");
	shutdown();
}
