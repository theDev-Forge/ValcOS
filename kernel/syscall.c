#include "syscall.h"
#include "process.h"
#include "fs.h"
#include "printk.h"
#include "signal.h"
#include "elf.h"
#include "memory.h"

// Heap management
static void *heap_end = (void *)0x80000000;  // Start heap at 2GB

void sys_exit(int status) {
    pr_info("Process %d exiting with status %d\n", current_process->pid, status);
    process_kill(current_process->pid);
    schedule();  // Never returns
}

int sys_fork(void) {
    return process_fork();
}

int sys_read(int fd, void *buf, size_t count) {
    return vfs_read(fd, buf, count);
}

int sys_write(int fd, const void *buf, size_t count) {
    // For now, write to console if fd is 1 (stdout)
    if (fd == 1) {
        for (size_t i = 0; i < count; i++) {
            pr_info("%c", ((char *)buf)[i]);
        }
        return count;
    }
    return vfs_write(fd, buf, count);
}

int sys_open(const char *path, int flags) {
    return vfs_open(path, flags);
}

int sys_close(int fd) {
    return vfs_close(fd);
}

int sys_waitpid(int pid, int *status, int options) {
    (void)options;
    return process_wait(pid, status);
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    (void)argv;
    (void)envp;
    return elf_exec(path);
}

int sys_getpid(void) {
    return current_process ? current_process->pid : 0;
}

int sys_brk(void *addr) {
    // Simple heap management
    if (addr == NULL) {
        return (int)heap_end;
    }
    
    // Validate address
    if (addr < (void *)0x80000000 || addr > (void *)0xC0000000) {
        return -1;  // Invalid address
    }
    
    heap_end = addr;
    return (int)heap_end;
}

void syscall_handler(registers_t *regs) {
    // Syscall number in EAX
    uint32_t syscall_num = regs->eax;
    
    // Arguments in EBX, ECX, EDX, ESI, EDI
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    
    int ret = -1;
    
    switch (syscall_num) {
        case SYS_EXIT:
            sys_exit((int)arg1);
            break;
        case SYS_FORK:
            ret = sys_fork();
            break;
        case SYS_READ:
            ret = sys_read((int)arg1, (void *)arg2, (size_t)arg3);
            break;
        case SYS_WRITE:
            ret = sys_write((int)arg1, (const void *)arg2, (size_t)arg3);
            break;
        case SYS_OPEN:
            ret = sys_open((const char *)arg1, (int)arg2);
            break;
        case SYS_CLOSE:
            ret = sys_close((int)arg1);
            break;
        case SYS_WAITPID:
            ret = sys_waitpid((int)arg1, (int *)arg2, (int)arg3);
            break;
        case SYS_EXECVE:
            ret = sys_execve((const char *)arg1, (char *const *)arg2, (char *const *)arg3);
            break;
        case SYS_GETPID:
            ret = sys_getpid();
            break;
        case SYS_KILL:
            ret = sys_kill((uint32_t)arg1, (int)arg2);
            break;
        case SYS_BRK:
            ret = sys_brk((void *)arg1);
            break;
        default:
            pr_warn("Unknown syscall: %d\n", syscall_num);
            ret = -1;
    }
    
    // Return value in EAX
    regs->eax = ret;
}

void init_syscalls(void) {
    pr_info("Syscall interface initialized\n");
}
