#include "fs.h"
#include "kernel.h"
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
	fsinit();
	Log("Ready to run user processs");
	shutdown();
}
