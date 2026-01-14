# ValcOS - Tool Installation Guide

## Required Tools

To build ValcOS, you need the following tools:

1. **NASM** - Netwide Assembler (for assembly code)
2. **GCC** - GNU Compiler Collection (for C code)
3. **LD** - GNU Linker (for linking)
4. **Make** - Build automation (optional, we have PowerShell alternative)
5. **QEMU** - Emulator for testing

## Installation on Windows

### Option 1: Using Winget (Recommended)

```powershell
# Install NASM
winget install nasm.nasm

# Install MSYS2 (includes GCC, LD, Make)
winget install MSYS2.MSYS2

# Install QEMU
winget install SoftwareFreedomConservancy.QEMU
```

After installing MSYS2:
1. Open "MSYS2 MINGW64" from Start Menu
2. Run: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils make`

### Option 2: Manual Installation

#### NASM
1. Download from: https://www.nasm.us/pub/nasm/releasebuilds/
2. Install and add to PATH

#### MinGW-w64 (GCC + LD)
1. Download from: https://www.mingw-w64.org/downloads/
2. Install and add `bin` folder to PATH
3. Verify with: `gcc --version`

#### Make
1. Download from: http://gnuwin32.sourceforge.net/packages/make.htm
2. Install and add to PATH

#### QEMU
1. Download from: https://www.qemu.org/download/#windows
2. Install and add to PATH

### Option 3: Using Chocolatey

```powershell
# Install Chocolatey first if you don't have it
# Then run:
choco install nasm
choco install mingw
choco install make
choco install qemu
```

## Verifying Installation

Open PowerShell and run:

```powershell
nasm -version
gcc --version
ld --version
make --version
qemu-system-x86_64 --version
```

All commands should return version information.

## Building ValcOS

Once tools are installed:

### Using Make (if installed)
```bash
make clean
make
make run
```

### Using PowerShell Script
```powershell
.\build.ps1
```

### Manual Build Commands

If the automated scripts don't work, you can build manually:

```powershell
# Create build directory
mkdir build

# Assemble bootloader
nasm -f bin boot/boot.asm -o build/boot.bin

# Assemble kernel entry
nasm -f elf32 kernel/kernel_entry.asm -o build/kernel_entry.o

# Compile C files
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude kernel/kernel.c -o build/kernel.o
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude kernel/idt.c -o build/idt.o
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude kernel/shell.c -o build/shell.o
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude kernel/string.c -o build/string.o
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude drivers/vga.c -o build/vga.o
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Iinclude drivers/keyboard.c -o build/keyboard.o

# Link kernel
ld -m elf_i386 -T linker.ld -o build/kernel.bin build/kernel_entry.o build/kernel.o build/idt.o build/shell.o build/string.o build/vga.o build/keyboard.o

# Create OS image (combine bootloader + kernel)
cmd /c "copy /b build\boot.bin+build\kernel.bin build\ValcOS.img"

# Pad to 1.44MB
fsutil file seteof build\ValcOS.img 1474560

# Run in QEMU
qemu-system-x86_64 -drive format=raw,file=build/ValcOS.img
```

## Troubleshooting

### "command not found" errors
- Make sure all tools are added to your PATH environment variable
- Restart your terminal after installation
- Try running from MSYS2 terminal instead of PowerShell

### GCC 32-bit support errors
- You need a compiler that supports `-m32` flag
- MinGW-w64 or MSYS2's GCC should work
- Alternatively, use a cross-compiler like `i686-elf-gcc`

### Linker errors
- Make sure you're using GNU ld, not MSVC link.exe
- Check that ld is in PATH before any Windows linker

### QEMU not starting
- Make sure QEMU is properly installed
- Try: `qemu-system-i386` instead of `qemu-system-x86_64`
- Check that the image file was created successfully

## Next Steps

Once you have the tools installed and ValcOS builds successfully:

1. Run `qemu-system-x86_64 -drive format=raw,file=build/ValcOS.img`
2. You should see the ValcOS welcome message
3. Try the shell commands: `help`, `about`, `clear`, `echo hello`
4. Press Ctrl+C in the terminal to exit QEMU

Happy OS development! 🚀
