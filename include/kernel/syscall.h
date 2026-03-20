#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include "types.h"

void syscall(void);

i32 kexec(const char *path, char **argv);

#endif
