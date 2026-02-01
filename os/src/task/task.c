#include <stddef.h>

#include "kernel/common.h"
#include "log.h"
#include "task/task.h"

__aligned(4096) KernelStack Kernel_Stack[] = (KernelStack[MAX_APP_NUM]){{{0}}};
__aligned(4096) UserStack User_Stack[] = (UserStack[MAX_APP_NUM]){{{0}}};

TaskManager *get_task_manager() {
    static TaskManager instance;
    static bool initialized = false;

    if (!initialized) {
        instance.num_app = get_num_app();
        instance.task_manager_inner.current_task_index = 0;

        for (size_t i = 0; i < MAX_APP_NUM; i++) {
            if (i < instance.num_app) {
                instance.task_manager_inner.tasks[i].task_cx = goto_restore(init_app_cx(i));
                instance.task_manager_inner.tasks[i].task_status = Ready;
            } else {
                instance.task_manager_inner.tasks[i].task_status = UnInit;
            }
        }
        initialized = true;
    }

    return &instance;
}

static size_t find_next_task(const TaskManager *task_manager) {
    TaskManagerInner inner = task_manager->task_manager_inner;
    size_t current = inner.current_task_index;
    size_t ret = SIZE_MAX;
    for (size_t i = current + 1; i < current + task_manager->num_app + 1; i++) {
        size_t idx = i % task_manager->num_app;
        if (inner.tasks[idx].task_status == Ready) {
            ret = idx;
            break;
        }
    }
    return ret;
}

void run_first_task() {
    TaskManager *task_manager = get_task_manager();
    TaskManagerInner *inner = &task_manager->task_manager_inner;
    TaskControlBlock *first_task = &inner->tasks[0];
    first_task->task_status = Running;
    TaskContext *next_task_cx = (TaskContext *)(&first_task->task_cx);
    TaskContext bootstrap_cx = init_task_context();
    __switch(&bootstrap_cx, next_task_cx);
}

void run_next_task() {
    TaskManager *task_manager = get_task_manager();
    if (!task_manager) {
        Error("Failed to get task manager!");
        shutdown();
    }
    size_t next = find_next_task(task_manager);
    if (next == SIZE_MAX) {
        Info("No task to run!");
        shutdown();
    }
    TaskManagerInner *inner = &task_manager->task_manager_inner;
    size_t current = inner->current_task_index;
    inner->tasks[next].task_status = Running;
    inner->current_task_index = next;
    TaskContext *current_task_cx = (TaskContext *)(&inner->tasks[current].task_cx);
    TaskContext *next_task_cx = (TaskContext *)(&inner->tasks[next].task_cx);
    __switch(current_task_cx, next_task_cx);
}

void mark_current_suspended() {
    TaskManager *taskmanager = get_task_manager();
    TaskManagerInner *inner = &taskmanager->task_manager_inner;
    size_t current = inner->current_task_index;
    inner->tasks[current].task_status = Ready;
}

void mark_current_exited() {
    TaskManager *taskmanager = get_task_manager();
    TaskManagerInner *inner = &taskmanager->task_manager_inner;
    size_t current = inner->current_task_index;
    inner->tasks[current].task_status = Exited;
}

void suspend_current_and_run_next() {
    mark_current_suspended();
    run_next_task();
}

void exit_current_and_run_next() {
    mark_current_exited();
    run_next_task();
}
