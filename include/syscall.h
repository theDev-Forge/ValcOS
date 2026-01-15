#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include "idt.h"

// Syscall numbers (Linux-compatible)
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_WAITPID 7
#define SYS_EXECVE  11
#define SYS_GETPID  20
#define SYS_KILL    37
#define SYS_BRK     45

void init_syscalls(void);
void syscall_handler(registers_t *regs);

// Syscall implementations
void sys_exit(int status);
int sys_fork(void);
int sys_read(int fd, void *buf, size_t count);
int sys_write(int fd, const void *buf, size_t count);
int sys_open(const char *path, int flags);
int sys_close(int fd);
int sys_waitpid(int pid, int *status, int options);
int sys_execve(const char *path, char *const argv[], char *const envp[]);
int sys_getpid(void);
int sys_brk(void *addr);

#endif
