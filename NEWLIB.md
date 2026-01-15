# Building newlib for ValcOS

## Prerequisites

1. **Cross-compiler toolchain** (i686-elf-gcc)
2. **newlib source** (download from newlib.sourceware.org)

## Build Steps

### Step 1: Configure newlib

```bash
cd newlib-4.3.0
mkdir build-valcos
cd build-valcos

../configure \
    --target=i686-elf \
    --prefix=/usr/local/i686-elf \
    --disable-multilib \
    --disable-newlib-supplied-syscalls
```

### Step 2: Build and install

```bash
make all
sudo make install
```

### Step 3: Link with ValcOS programs

```bash
i686-elf-gcc -c userspace/newlib_stubs.c -o build/newlib_stubs.o

i686-elf-gcc \
    -nostdlib \
    -T userspace/linker.ld \
    userspace/hello.c \
    build/newlib_stubs.o \
    -o build/hello.elf \
    -lc -lgcc
```

## Testing

Create a test program:

```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from newlib!\n");
    return 0;
}
```

Compile and run in ValcOS!

## Phase 12 Status

✅ newlib syscall stubs created
✅ Build instructions documented
⏳ Needs: Cross-compiled newlib library
⏳ Needs: Testing with real programs

**Next**: Compile newlib and test with printf/malloc programs
