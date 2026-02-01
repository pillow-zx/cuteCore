#ifndef __DEFS_H_
#define __DEFS_H_

#include <stdarg.h>
#include <stdint.h>

#include "generated/autoconf.h"
#include "utils.h"
#include "task.h"


void shutdown();

// uart.c
void uart_init();
void uart_write(const char *buf, int len);
void uart_read(char *buf, int limit);

// console.c
__format(printf, 1, 2) int32_t printf(const char *fmt, ...);

// trap.c
void trap_init();
void trap_enable_timer_interrupt();

// timer.c
__maybe_unused uint64_t get_time();
__maybe_unused uint64_t get_time_ms();
__maybe_unused void set_next_timer();

// syscall.c
int64_t syscall(uint64_t syscall_id, uint64_t args[]);

// fs.c
int64_t sys_write(const uint64_t fd, const char *buf, uint64_t len);

// sys_process.c
__noreturn void sys_exit(int64_t code);
intptr_t sys_yield();
intptr_t sys_get_time();

// task.c
void run_first_task();
void run_next_task();
void mark_current_suspended();
void mark_current_exited();
void suspend_current_and_run_next();
void exit_current_and_run_next();

// loader.c
void load_apps();

// switch.S
void __switch(TaskContext *old, TaskContext *new);


#endif // __DEFS_H_
