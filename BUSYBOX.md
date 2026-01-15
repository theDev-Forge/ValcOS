# Complete Guide: Building BusyBox for ValcOS

This guide walks you through compiling BusyBox and integrating it with ValcOS to get 300+ Unix utilities.

---

## Prerequisites

### 1. Cross-Compiler Toolchain

You need `i686-elf-gcc` (32-bit x86 cross-compiler).

#### Option A: Install Pre-built (Recommended)
```bash
# On Windows with MSYS2/MinGW64:
pacman -S mingw-w64-i686-gcc

# On Linux (Ubuntu/Debian):
sudo apt-get install gcc-multilib binutils-dev

# On macOS:
brew install i686-elf-gcc
```

#### Option B: Build from Source
Follow the guide at: https://wiki.osdev.org/GCC_Cross-Compiler

**Verify installation:**
```bash
i686-elf-gcc --version
i686-elf-ld --version
```

---

## Part 1: Building newlib (C Library)

BusyBox requires a C library. We'll use newlib.

### Step 1: Download newlib

```bash
cd ~/Downloads
wget ftp://sourceware.org/pub/newlib/newlib-4.3.0.tar.gz
tar xzf newlib-4.3.0.tar.gz
cd newlib-4.3.0
```

### Step 2: Configure newlib for ValcOS

```bash
mkdir build-valcos
cd build-valcos

../configure \
    --target=i686-elf \
    --prefix=/usr/local/i686-elf \
    --disable-multilib \
    --disable-newlib-supplied-syscalls \
    --enable-newlib-reent-small \
    --enable-newlib-io-long-long \
    --enable-newlib-io-c99-formats
```

**Configuration options explained:**
- `--target=i686-elf`: Build for 32-bit x86
- `--prefix=/usr/local/i686-elf`: Install location
- `--disable-newlib-supplied-syscalls`: We provide our own syscalls
- `--enable-newlib-reent-small`: Smaller memory footprint
- `--enable-newlib-io-*`: Enable modern I/O features

### Step 3: Build and Install

```bash
# Build (takes 5-10 minutes)
make all -j$(nproc)

# Install (requires sudo)
sudo make install
```

**Verify installation:**
```bash
ls /usr/local/i686-elf/lib/libc.a
ls /usr/local/i686-elf/include/stdio.h
```

### Step 4: Copy ValcOS Syscall Stubs

```bash
# Copy our syscall stubs to newlib
cd /path/to/ValcOS
cp userspace/newlib_stubs.c /usr/local/i686-elf/lib/
cd /usr/local/i686-elf/lib/

# Compile stubs
i686-elf-gcc -c newlib_stubs.c -o newlib_stubs.o

# Create combined library
i686-elf-ar rcs libvalcos.a newlib_stubs.o
```

---

## Part 2: Building BusyBox

### Step 1: Download BusyBox

```bash
cd ~/Downloads
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xjf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
```

### Step 2: Configure BusyBox

```bash
make menuconfig
```

**Required Settings:**

1. **Settings → Build Options**
   - ✅ Enable "Build static binary (no shared libs)"
   - ✅ Enable "Build with Large File Support"
   - Cross compiler prefix: `i686-elf-`
   - Additional CFLAGS: `-m32 -ffreestanding -nostdlib`
   - Additional LDFLAGS: `-static -nostdlib`

2. **Settings → Library Tuning**
   - ✅ Enable "Use sendfile system call"
   - ✅ Enable "Support POSIX threads"

3. **Disable Networking (for now)**
   - ❌ Networking Utilities → Disable all

4. **Enable Core Utilities**
   - ✅ Coreutils → Enable: ls, cat, echo, cp, mv, rm, mkdir, etc.
   - ✅ Editors → Enable: vi
   - ✅ Finding Utilities → Enable: grep, find
   - ✅ Init Utilities → Enable: init
   - ✅ Shell → Enable: ash (BusyBox shell)

**Save configuration as `.config`**

### Step 3: Create BusyBox Build Script

Create `build_busybox.sh`:

```bash
#!/bin/bash
#
# Build BusyBox for ValcOS
#

set -e

export CROSS_COMPILE=i686-elf-
export CC=i686-elf-gcc
export LD=i686-elf-ld
export AR=i686-elf-ar

# Compiler flags
export CFLAGS="-m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2"
export LDFLAGS="-static -nostdlib -T /path/to/ValcOS/userspace/linker.ld"

# Libraries
export LIBS="/usr/local/i686-elf/lib/libc.a /usr/local/i686-elf/lib/libvalcos.a"

echo "Building BusyBox for ValcOS..."

# Clean previous build
make clean

# Build
make -j$(nproc) \
    CROSS_COMPILE=$CROSS_COMPILE \
    EXTRA_CFLAGS="$CFLAGS" \
    EXTRA_LDFLAGS="$LDFLAGS" \
    EXTRA_LDLIBS="$LIBS"

echo "BusyBox built successfully!"
echo "Output: busybox"

# Strip symbols to reduce size
i686-elf-strip busybox

# Show size
ls -lh busybox
```

Make it executable:
```bash
chmod +x build_busybox.sh
```

### Step 4: Build BusyBox

```bash
./build_busybox.sh
```

**Expected output:**
```
Building BusyBox for ValcOS...
...
BusyBox built successfully!
Output: busybox
-rwxr-xr-x 1 user user 1.2M busybox
```

---

## Part 3: Integrating BusyBox with ValcOS

### Step 1: Copy BusyBox to ValcOS

```bash
cd /path/to/ValcOS
mkdir -p rootfs/bin
cp ~/Downloads/busybox-1.36.1/busybox rootfs/bin/
```

### Step 2: Create Symlinks

BusyBox uses symlinks for different commands:

```bash
cd rootfs/bin

# Create symlinks for common utilities
ln -s busybox ls
ln -s busybox cat
ln -s busybox echo
ln -s busybox cp
ln -s busybox mv
ln -s busybox rm
ln -s busybox mkdir
ln -s busybox rmdir
ln -s busybox grep
ln -s busybox sed
ln -s busybox awk
ln -s busybox vi
ln -s busybox sh
ln -s busybox ps
ln -s busybox top
ln -s busybox kill
```

Or use BusyBox's built-in installer:
```bash
./busybox --install -s .
```

### Step 3: Update FAT12 Image

Modify `scripts/make_fat12.py` to include the `rootfs` directory:

```python
# Add after creating filesystem
import shutil

# Copy rootfs to disk image
if os.path.exists('rootfs'):
    for root, dirs, files in os.walk('rootfs'):
        for file in files:
            src = os.path.join(root, file)
            dst = src.replace('rootfs/', '')
            # Copy to FAT12 image
            # (implementation depends on your FAT12 tool)
```

### Step 4: Rebuild ValcOS

```bash
make clean
make all
make run
```

---

## Part 4: Testing BusyBox in ValcOS

Once ValcOS boots:

```
ValcOS> ls /bin
busybox  cat  cp  echo  ls  mkdir  mv  rm  sh  vi

ValcOS> exec /bin/ls
(BusyBox ls runs)

ValcOS> exec /bin/sh
BusyBox v1.36.1 (2026-01-15) built-in shell (ash)
/ # ls
/ # echo "Hello from BusyBox!"
Hello from BusyBox!
/ # exit
```

---

## Troubleshooting

### Issue: "undefined reference to `_sbrk`"
**Solution:** Make sure `newlib_stubs.c` is compiled and linked.

### Issue: "cannot find -lc"
**Solution:** Verify newlib is installed:
```bash
ls /usr/local/i686-elf/lib/libc.a
```

### Issue: BusyBox crashes on startup
**Solution:** Check that:
1. ELF loader is working (`exec` command works)
2. Syscalls are implemented (especially `write`, `exit`)
3. User stack is set up correctly

### Issue: "Illegal instruction"
**Solution:** Make sure you're compiling for i686 (32-bit), not x86_64.

---

## Advanced: Minimal BusyBox Configuration

For a smaller binary, use this minimal config:

```bash
make allnoconfig
make menuconfig
```

Enable only:
- Coreutils: ls, cat, echo, cp, mv, rm, mkdir
- Shell: ash
- Init: init

This produces a ~200KB binary instead of 1.2MB.

---

## Next Steps

Once BusyBox works:

1. **Add more utilities** - Enable networking, text processing, etc.
2. **Create init scripts** - Auto-start services on boot
3. **Port more software** - vim, gcc, make
4. **Implement EXT2** - Better filesystem than FAT12

---

## Summary

**What you built:**
- ✅ Cross-compiler toolchain
- ✅ newlib C library for ValcOS
- ✅ BusyBox with 300+ Unix utilities
- ✅ Integrated into ValcOS filesystem

**What you can do:**
- Run Unix commands in ValcOS
- Use BusyBox shell
- Create shell scripts
- Build more complex applications

**Congratulations!** You now have a fully functional Unix-like operating system! 🎉
