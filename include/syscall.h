#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_DUP 23
#define SYS_DUP3 24
#define SYS_UMOUNT2 39
#define SYS_MOUNT 40
#define SYS_OPENAT 56
#define SYS_CLOSE 57
#define SYS_PIPE2 59
#define SYS_READ 63
#define SYS_WRITE 64
#define SYS_EXIT 93
#define SYS_WAITTID 95
#define SYS_KILL 129
#define SYS_TKILL 130
#define SYS_TGKILL 131
#define SYS_UNAME 160
#define SYS_GETPID 172
#define SYS_GETPPID 173
#define SYS_GETUID 174
#define SYS_BRK 214
#define SYS_EXECVE 221
#define SYS_MMAP 222
#define SYS_WAIT4 260
#define SYS_GETRANDOM 278

#endif
