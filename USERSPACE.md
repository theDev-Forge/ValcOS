# Building and Running Userspace Programs on ValcOS

## Prerequisites

You need a cross-compiler for i686 (32-bit x86):

### Option 1: Install pre-built toolchain
```bash
# On Windows with MSYS2/MinGW:
pacman -S mingw-w64-i686-gcc

# On Linux:
sudo apt-get install gcc-multilib
```

### Option 2: Build your own cross-compiler
Follow: https://wiki.osdev.org/GCC_Cross-Compiler

## Building the Test Program

### Step 1: Compile hello.c
```bash
i686-elf-gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c userspace/hello.c -o build/hello.o
```

### Step 2: Link
```bash
i686-elf-ld -m elf_i386 -T userspace/linker.ld build/hello.o -o build/hello.elf
```

### Step 3: Verify it's a valid ELF
```bash
file build/hello.elf
# Should show: ELF 32-bit LSB executable, Intel 80386
```

## Adding to ValcOS Filesystem

### Step 1: Copy to rootfs
```bash
cp build/hello.elf rootfs/hello.elf
```

### Step 2: Rebuild disk image
```bash
make run
```

### Step 3: Run in ValcOS
```
ValcOS> ls
ValcOS> exec /hello.elf
```

## Current Limitations

The ELF loader is **partially implemented**:
- ✅ Can parse ELF headers
- ✅ Can identify loadable segments
- ❌ Cannot allocate user memory yet (needs VMM integration)
- ❌ Cannot switch to user mode yet (needs ring 3 setup)
- ❌ Cannot handle syscalls yet

## Next Steps

To fully run programs, you need to implement:
1. **User mode switching** (ring 0 → ring 3)
2. **VMM integration** for user memory allocation
3. **System calls** for program interaction
4. **User stack setup**

This is Phase 10 completion + Phase 11 (syscalls).

## Expected Timeline

- **Phase 10 completion**: 1-2 weeks
- **Phase 11 (syscalls)**: 1 week
- **Phase 12 (newlib)**: 1-2 weeks

After that, you can run real programs with printf, malloc, etc.!
