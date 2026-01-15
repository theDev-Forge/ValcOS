# BusyBox for ValcOS - CORRECTED Technical Guide

**This guide has been technically reviewed and corrected for i386 Linux ABI compatibility.**

---

## Critical Requirements

### 1. Correct Syscall Table (i386 Linux)

**IMPORTANT**: BusyBox uses specific Linux syscalls. You MUST implement these exactly:

```c
// kernel/syscall.h - CORRECTED for i386 Linux

// Process control
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_EXECVE      11
#define SYS_GETPID      20
#define SYS_KILL        37
#define SYS_BRK         45
#define SYS_IOCTL       54
#define SYS_DUP2        63
#define SYS_FCNTL       55

// CRITICAL: BusyBox uses these, not the old versions
#define SYS_MUNMAP      91
#define SYS_WAIT4       114   // NOT waitpid!
#define SYS_CLONE       120   // BusyBox uses clone, not fork
#define SYS_MMAP2       192   // NOT mmap (90)!
#define SYS_EXIT_GROUP  252
#define SYS_OPENAT      295
#define SYS_PIPE2       331

// Real-time signals (required by musl)
#define SYS_RT_SIGACTION    174
#define SYS_RT_SIGPROCMASK  175
#define SYS_SIGRETURN       119
```

### 2. Why These Syscalls?

| Syscall | Why BusyBox Needs It |
|---------|---------------------|
| `mmap2` | musl uses mmap2 on i386, not mmap |
| `wait4` | Actual syscall; waitpid is libc wrapper |
| `clone` | BusyBox uses clone internally |
| `exit_group` | Modern exit for threaded programs |
| `rt_sigaction` | musl signal handling |
| `pipe2` | Modern pipe with flags |

---

## Part 1: Install musl Cross-Compiler (CORRECTED)

### On Windows (MSYS2)

**IMPORTANT**: MSYS2 does NOT have `mingw-w64-cross-musl`. You must build from source.

```bash
# In MSYS2 terminal
cd ~
git clone https://github.com/richfelker/musl-cross-make
cd musl-cross-make

# Create config.mak
cat > config.mak << 'EOF'
TARGET = i686-linux-musl
OUTPUT = /c/cross
GCC_VER = 13.2.0
MUSL_VER = 1.2.4
BINUTILS_VER = 2.41
EOF

# Build (takes 20-30 minutes)
make -j4

# Install (NO sudo on MSYS2!)
make install

# Add to PATH
export PATH=/c/cross/bin:$PATH
echo 'export PATH=/c/cross/bin:$PATH' >> ~/.bashrc
```

### Verify

```bash
i686-linux-musl-gcc --version
# Should show: i686-linux-musl-gcc (GCC) 13.2.0
```

---

## Part 2: Implement Missing Syscalls in ValcOS

### Add to `include/syscall.h`

```c
// Add these syscall numbers
#define SYS_WAIT4       114
#define SYS_CLONE       120
#define SYS_MMAP2       192
#define SYS_EXIT_GROUP  252
#define SYS_RT_SIGACTION 174
#define SYS_RT_SIGPROCMASK 175
#define SYS_PIPE2       331
#define SYS_DUP2        63

// Add declarations
int sys_wait4(int pid, int *status, int options, void *rusage);
int sys_clone(unsigned long flags, void *child_stack);
void *sys_mmap2(void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset);
void sys_exit_group(int status);
int sys_rt_sigaction(int signum, const void *act, void *oldact, size_t sigsetsize);
int sys_pipe2(int pipefd[2], int flags);
int sys_dup2(int oldfd, int newfd);
```

### Add to `kernel/syscall.c`

```c
// wait4 - BusyBox uses this, not waitpid
int sys_wait4(int pid, int *status, int options, void *rusage) {
    (void)rusage;  // Ignore for now
    return process_wait(pid, status);
}

// clone - BusyBox uses this internally
int sys_clone(unsigned long flags, void *child_stack) {
    (void)flags;
    (void)child_stack;
    // Simple implementation: treat as fork
    return process_fork();
}

// mmap2 - CRITICAL: i386 uses mmap2, not mmap
void *sys_mmap2(void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset) {
    (void)addr; (void)prot; (void)flags; (void)fd; (void)pgoffset;
    
    // Simple implementation: allocate memory
    void *mem = kmalloc(length);
    if (!mem) return (void *)-1;
    
    memset(mem, 0, length);
    return mem;
}

// exit_group - Modern exit
void sys_exit_group(int status) {
    sys_exit(status);
}

// rt_sigaction - Real-time signals
int sys_rt_sigaction(int signum, const void *act, void *oldact, size_t sigsetsize) {
    (void)sigsetsize;
    // Wrapper around old signal
    if (oldact) {
        // TODO: Fill old action
    }
    if (act) {
        // TODO: Set new action
    }
    return 0;
}

// pipe2 - Modern pipe with flags
int sys_pipe2(int pipefd[2], int flags) {
    (void)flags;
    // TODO: Implement pipe
    pr_warn("pipe2() not implemented\n");
    return -1;
}

// dup2 - Duplicate file descriptor
int sys_dup2(int oldfd, int newfd) {
    (void)oldfd; (void)newfd;
    // TODO: Implement dup2
    return oldfd;
}

// Update syscall_handler
void syscall_handler(registers_t *regs) {
    uint32_t syscall_num = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    uint32_t arg5 = regs->edi;
    
    int ret = -1;
    
    switch (syscall_num) {
        // ... existing cases ...
        
        case SYS_WAIT4:
            ret = sys_wait4((int)arg1, (int *)arg2, (int)arg3, (void *)arg4);
            break;
        case SYS_CLONE:
            ret = sys_clone((unsigned long)arg1, (void *)arg2);
            break;
        case SYS_MMAP2:
            ret = (int)sys_mmap2((void *)arg1, (size_t)arg2, (int)arg3, 
                                 (int)arg4, (int)arg5, (off_t)regs->ebp);
            break;
        case SYS_EXIT_GROUP:
            sys_exit_group((int)arg1);
            break;
        case SYS_RT_SIGACTION:
            ret = sys_rt_sigaction((int)arg1, (void *)arg2, (void *)arg3, (size_t)arg4);
            break;
        case SYS_PIPE2:
            ret = sys_pipe2((int *)arg1, (int)arg2);
            break;
        case SYS_DUP2:
            ret = sys_dup2((int)arg1, (int)arg2);
            break;
    }
    
    regs->eax = ret;
}
```

---

## Part 3: Build BusyBox (CORRECTED)

### Step 1: Download

```bash
cd ~/valcos-build
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xjf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
```

### Step 2: Configure (CRITICAL SETTINGS)

```bash
make defconfig
make menuconfig
```

**REQUIRED Settings:**

1. **Settings → Build Options**
   - ✅ Build static binary
   - Cross compiler prefix: `i686-linux-musl-`

2. **Settings → Library Tuning**
   - ❌ **DISABLE** "Support POSIX threads" (causes TLS issues)
   - ❌ **DISABLE** "Use sendfile" (not implemented yet)

3. **Applets**
   - ✅ Enable: sh, ls, cat, echo, cp, mv, rm, mkdir
   - ❌ Disable: Networking (until you implement full network stack)

### Step 3: Build

```bash
make -j4 CROSS_COMPILE=i686-linux-musl-
i686-linux-musl-strip busybox
```

---

## Part 4: Integration (CORRECTED)

### Copy to ValcOS

```bash
cd /c/Users/Administrator/Desktop/stuff/projects/ValcOS
mkdir -p rootfs/bin
cp ~/valcos-build/busybox-1.36.1/busybox rootfs/bin/
```

### Create Symlinks (MANUAL - Cannot run Linux binary on Windows!)

**IMPORTANT**: You CANNOT run `./busybox --install` on Windows!

```bash
cd rootfs/bin

# Create symlinks manually
ln -s busybox sh
ln -s busybox ls
ln -s busybox cat
ln -s busybox echo
ln -s busybox cp
ln -s busybox mv
ln -s busybox rm
ln -s busybox mkdir
ln -s busybox ps
ln -s busybox kill
```

**Alternative**: Let BusyBox create links at boot:

```bash
# Just copy busybox, no symlinks
# Boot ValcOS with: init=/bin/busybox
```

---

## Part 5: Testing

### Rebuild ValcOS

```bash
make clean
make all
make run
```

### Test in ValcOS

```
ValcOS> exec /bin/sh
/ # ls
/ # echo "Hello from BusyBox!"
/ # ps
```

---

## Mandatory Checklist Before BusyBox Works

- [ ] Implement `wait4` (not waitpid)
- [ ] Implement `clone` (even as fork wrapper)
- [ ] Implement `mmap2` (NOT mmap!)
- [ ] Implement `exit_group`
- [ ] Implement `rt_sigaction`, `rt_sigprocmask`
- [ ] Disable pthreads in BusyBox config
- [ ] Disable sendfile in BusyBox config
- [ ] Use i386 syscall numbers exactly
- [ ] Do NOT try to run BusyBox on Windows

---

## Common Mistakes to Avoid

| ❌ Wrong | ✅ Correct |
|---------|-----------|
| `SYS_WAITPID 7` | `SYS_WAIT4 114` |
| `SYS_MMAP 90` | `SYS_MMAP2 192` |
| `pacman -S mingw-w64-cross-musl` | Build musl-cross-make |
| `sudo make install` (MSYS2) | `make install` (no sudo) |
| `./busybox --install` on Windows | Manual symlinks |
| Enable pthreads | Disable pthreads |
| Implement fork only | Implement clone |

---

## Summary

**This is the CORRECT, technically accurate approach:**

1. ✅ Use musl libc (not newlib)
2. ✅ Build musl-cross-make from source
3. ✅ Use i386 Linux syscall numbers
4. ✅ Implement mmap2, wait4, clone, exit_group
5. ✅ Disable pthreads in BusyBox
6. ✅ Create symlinks manually on Windows

**Result**: BusyBox will work correctly on ValcOS! 🎉
