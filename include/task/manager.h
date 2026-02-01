#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__

#include "task.h"

// task.c
void run_first_task();
void run_next_task();
void mark_current_suspended();
void mark_current_exited();
void suspend_current_and_run_next();
void exit_current_and_run_next();

// switch.S
void __switch(TaskContext *old, TaskContext *new);

#endif // __TASK_MANAGER_H__