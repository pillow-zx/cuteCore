#ifndef __TASK_H__
#define __TASK_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/riscv/trap.h"
#include "config.h"
#include "utils/utils.h"

typedef struct {
    uint8_t data[KERNEL_STACK_SIZE];
} KernelStack;

typedef struct {
    uint8_t data[USER_STACK_SIZE];
} UserStack;

enum TaskStatus {
    UnInit,
    Ready,
    Running,
    Exited,
};

typedef struct {
    size_t ra;
    size_t sp;
    size_t s[12];
} TaskContext;

typedef struct {
    enum TaskStatus task_status;
    TaskContext task_cx;
} TaskControlBlock;

typedef struct {
    TaskControlBlock tasks[MAX_APP_NUM];
    size_t current_task_index;
} TaskManagerInner;

typedef struct {
    size_t num_app;
    TaskManagerInner task_manager_inner;
} TaskManager;

extern KernelStack Kernel_Stack[];
extern UserStack User_Stack[];

__always_inline size_t kernel_stack_sp(KernelStack *kernel_stack) {
    return (size_t)(kernel_stack->data + KERNEL_STACK_SIZE);
}

__inline size_t kernel_stack_push(KernelStack *kernel_stack, TrapContext *trap_cx) {
    size_t sp = kernel_stack_sp(kernel_stack);
    sp -= sizeof(TrapContext);
    *(TrapContext *)sp = *trap_cx;
    return sp;
}

__always_inline size_t user_stack_sp(UserStack *user_stack) { return (size_t)(user_stack->data + USER_STACK_SIZE); }

__always_inline size_t get_num_app() {
    extern uint8_t _num_app[];
    return *(volatile size_t *)_num_app;
}

__always_inline size_t get_base_i(size_t i) { return APP_BASE_ADDRESS + i * APP_SIZE_LIMIT; }

__always_inline size_t init_app_cx(size_t app_id) {
    TrapContext tc = app_init_context(get_base_i(app_id), user_stack_sp(&User_Stack[app_id]));
    return kernel_stack_push(&Kernel_Stack[app_id], &tc);
}
__always_inline TaskContext init_task_context() {
    return (TaskContext){
        .ra = 0,
        .sp = 0,
        .s = {0},
    };
};

extern uint8_t __restore[];

__always_inline TaskContext goto_restore(size_t kstack_ptr) {
    return (TaskContext){
        .ra = (size_t)__restore,
        .sp = kstack_ptr,
        .s = {0},
    };
};

TaskManager *get_task_manager();

#endif // TASK_H
