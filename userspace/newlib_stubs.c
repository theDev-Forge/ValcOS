/*
 * newlib syscall stubs for ValcOS
 * 
 * These stubs allow newlib to work with ValcOS syscalls
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>

// Syscall numbers
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_BRK     45

// Make syscall
static inline int syscall(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
    return ret;
}

void _exit(int status) {
    syscall(SYS_EXIT, status, 0, 0);
    while(1);  // Never returns
}

int _close(int file) {
    return syscall(SYS_CLOSE, file, 0, 0);
}

int _execve(char *name, char **argv, char **env) {
    (void)name; (void)argv; (void)env;
    errno = ENOMEM;
    return -1;
}

int _fork(void) {
    return syscall(SYS_FORK, 0, 0, 0);
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _getpid(void) {
    return 1;
}

int _isatty(int file) {
    return file <= 2;  // stdin, stdout, stderr
}

int _kill(int pid, int sig) {
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _link(char *old, char *new) {
    (void)old; (void)new;
    errno = EMLINK;
    return -1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file; (void)ptr; (void)dir;
    return 0;
}

int _open(const char *name, int flags, int mode) {
    return syscall(SYS_OPEN, (int)name, flags, mode);
}

int _read(int file, char *ptr, int len) {
    return syscall(SYS_READ, file, (int)ptr, len);
}

void *_sbrk(int incr) {
    static char *heap_end = 0;
    char *prev_heap_end;
    
    if (heap_end == 0) {
        heap_end = (char *)syscall(SYS_BRK, 0, 0, 0);
    }
    
    prev_heap_end = heap_end;
    
    if (syscall(SYS_BRK, (int)(heap_end + incr), 0, 0) < 0) {
        errno = ENOMEM;
        return (void *)-1;
    }
    
    heap_end += incr;
    return (void *)prev_heap_end;
}

int _stat(const char *file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _times(struct tms *buf) {
    (void)buf;
    return -1;
}

int _unlink(char *name) {
    (void)name;
    errno = ENOENT;
    return -1;
}

int _wait(int *status) {
    (void)status;
    errno = ECHILD;
    return -1;
}

int _write(int file, char *ptr, int len) {
    return syscall(SYS_WRITE, file, (int)ptr, len);
}
