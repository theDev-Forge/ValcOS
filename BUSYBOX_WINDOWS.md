# Building BusyBox for ValcOS on Windows

This guide is specifically for Windows users using MSYS2/MinGW64.

---

## Prerequisites for Windows

### 1. Install MSYS2

Download and install MSYS2 from: https://www.msys2.org/

After installation, open **MSYS2 MinGW 64-bit** terminal.

### 2. Install Required Packages

```bash
# Update package database
pacman -Syu

# Install development tools
pacman -S base-devel
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-i686-gcc
pacman -S mingw-w64-x86_64-binutils
pacman -S make
pacman -S nasm
pacman -S wget
pacman -S tar
```

### 3. Install Cross-Compiler

```bash
# Install i686-elf toolchain
pacman -S mingw-w64-i686-toolchain

# Verify installation
i686-w64-mingw32-gcc --version
```

**Note**: On Windows, the cross-compiler is `i686-w64-mingw32-gcc` instead of `i686-elf-gcc`.

---

## Part 1: Building newlib on Windows

### Step 1: Download newlib

Open MSYS2 MinGW terminal:

```bash
cd ~
mkdir valcos-build
cd valcos-build

# Download newlib
wget ftp://sourceware.org/pub/newlib/newlib-4.3.0.tar.gz
tar xzf newlib-4.3.0.tar.gz
cd newlib-4.3.0
```

### Step 2: Configure for Windows

```bash
mkdir build-valcos
cd build-valcos

../configure \
    --target=i686-elf \
    --prefix=/mingw64/i686-elf \
    --disable-multilib \
    --disable-newlib-supplied-syscalls \
    --enable-newlib-reent-small
```

### Step 3: Build and Install

```bash
# Build (takes 5-10 minutes)
make all -j4

# Install (no sudo needed on MSYS2)
make install
```

### Step 4: Build Syscall Stubs

```bash
cd /mingw64/i686-elf/lib

# Copy stubs from ValcOS (adjust path)
cp /c/Users/Administrator/Desktop/stuff/projects/ValcOS/userspace/newlib_stubs.c .

# Compile
i686-w64-mingw32-gcc -c newlib_stubs.c -o newlib_stubs.o

# Create library
i686-w64-mingw32-ar rcs libvalcos.a newlib_stubs.o
```

---

## Part 2: Building BusyBox on Windows

### Step 1: Download BusyBox

```bash
cd ~/valcos-build

wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xjf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
```

### Step 2: Configure BusyBox

```bash
make menuconfig
```

**Settings:**
1. Build Options → Build static binary: **YES**
2. Cross compiler prefix: `i686-w64-mingw32-`
3. Additional CFLAGS: `-m32 -ffreestanding -nostdlib`

Save and exit.

### Step 3: Build BusyBox

Create `build.sh` in the busybox directory:

```bash
#!/bin/bash
export CROSS_COMPILE=i686-w64-mingw32-
export CC=i686-w64-mingw32-gcc
export CFLAGS="-m32 -ffreestanding -nostdlib -fno-pie -O2"
export LDFLAGS="-static -nostdlib"
export LIBS="/mingw64/i686-elf/lib/libc.a /mingw64/i686-elf/lib/libvalcos.a"

make clean
make -j4 \
    CROSS_COMPILE=$CROSS_COMPILE \
    EXTRA_CFLAGS="$CFLAGS" \
    EXTRA_LDFLAGS="$LDFLAGS" \
    EXTRA_LDLIBS="$LIBS"

i686-w64-mingw32-strip busybox
echo "Done! Size: $(du -h busybox | cut -f1)"
```

Run it:
```bash
chmod +x build.sh
./build.sh
```

---

## Part 3: Integration with ValcOS

### Step 1: Copy to ValcOS (Windows paths)

```bash
# In MSYS2 terminal
cd /c/Users/Administrator/Desktop/stuff/projects/ValcOS
mkdir -p rootfs/bin

# Copy BusyBox
cp ~/valcos-build/busybox-1.36.1/busybox rootfs/bin/
```

### Step 2: Create Symlinks

```bash
cd rootfs/bin
./busybox --install -s .
```

### Step 3: Rebuild ValcOS

Back in Windows PowerShell or MSYS2:

```bash
cd /c/Users/Administrator/Desktop/stuff/projects/ValcOS
make clean
make all
make run
```

---

## Alternative: PowerShell Script

Create `Build-BusyBox.ps1`:

```powershell
# Build BusyBox for ValcOS on Windows
# Run this in PowerShell

$ErrorActionPreference = "Stop"

$VALCOS_DIR = "C:\Users\Administrator\Desktop\stuff\projects\ValcOS"
$BUILD_DIR = "$env:USERPROFILE\valcos-build"

Write-Host "=== ValcOS BusyBox Builder (Windows) ===" -ForegroundColor Cyan

# Check if MSYS2 is installed
if (-not (Test-Path "C:\msys64")) {
    Write-Host "ERROR: MSYS2 not found!" -ForegroundColor Red
    Write-Host "Please install MSYS2 from https://www.msys2.org/"
    exit 1
}

# Create build directory
New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null

Write-Host "Opening MSYS2 terminal..." -ForegroundColor Yellow
Write-Host "Please run the following commands in MSYS2:" -ForegroundColor Yellow
Write-Host ""
Write-Host "cd ~/valcos-build" -ForegroundColor Green
Write-Host "wget ftp://sourceware.org/pub/newlib/newlib-4.3.0.tar.gz" -ForegroundColor Green
Write-Host "tar xzf newlib-4.3.0.tar.gz" -ForegroundColor Green
Write-Host "cd newlib-4.3.0" -ForegroundColor Green
Write-Host "mkdir build && cd build" -ForegroundColor Green
Write-Host "../configure --target=i686-elf --prefix=/mingw64/i686-elf --disable-multilib" -ForegroundColor Green
Write-Host "make all -j4 && make install" -ForegroundColor Green
Write-Host ""

# Open MSYS2
Start-Process "C:\msys64\msys2_shell.cmd" -ArgumentList "-mingw64"

Write-Host "After newlib is built, press any key to continue..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

Write-Host "Build complete! Check MSYS2 terminal for results." -ForegroundColor Cyan
```

---

## Troubleshooting on Windows

### Issue: "command not found"
**Solution:** Make sure you're using **MSYS2 MinGW 64-bit** terminal, not regular Command Prompt.

### Issue: Path issues with `/c/Users/...`
**Solution:** In MSYS2, Windows paths are:
- `C:\Users\...` becomes `/c/Users/...`
- `D:\Projects\...` becomes `/d/Projects/...`

### Issue: "Permission denied"
**Solution:** No `sudo` needed on Windows. Just run commands directly.

### Issue: Cross-compiler not found
**Solution:** Install with:
```bash
pacman -S mingw-w64-i686-toolchain
```

---

## Quick Start (Windows)

1. **Install MSYS2** from https://www.msys2.org/
2. **Open MSYS2 MinGW 64-bit** terminal
3. **Run these commands:**

```bash
# Install tools
pacman -S base-devel mingw-w64-i686-toolchain make nasm wget

# Build newlib
cd ~
mkdir valcos-build && cd valcos-build
wget ftp://sourceware.org/pub/newlib/newlib-4.3.0.tar.gz
tar xzf newlib-4.3.0.tar.gz
cd newlib-4.3.0
mkdir build && cd build
../configure --target=i686-elf --prefix=/mingw64/i686-elf --disable-multilib
make all -j4 && make install

# Build BusyBox
cd ~/valcos-build
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xjf busybox-1.36.1.tar.bz2
cd busybox-1.36.1
make defconfig
# Edit .config to enable static build
make -j4 CROSS_COMPILE=i686-w64-mingw32-

# Copy to ValcOS
cp busybox /c/Users/Administrator/Desktop/stuff/projects/ValcOS/rootfs/bin/
```

4. **Rebuild ValcOS** in your regular terminal

---

## Summary

**Windows-specific differences:**
- Use MSYS2 instead of native Linux
- Cross-compiler: `i686-w64-mingw32-gcc` instead of `i686-elf-gcc`
- No `sudo` needed
- Paths use `/c/` instead of `C:\`

**Everything else is the same!** 🎉
