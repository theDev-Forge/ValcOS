# ValcOS - Quick Start Guide

## Current Status

✅ **Project Structure Created**  
✅ **All Source Code Written**  
✅ **Build System Configured**  
✅ **Documentation Complete**  

⚠️ **Build Tools Required** - You need to install the build tools before you can compile ValcOS.

## Next Steps

### 1. Install Required Tools

You need these tools to build ValcOS:
- **NASM** - Assembly compiler
- **GCC** - C compiler (with 32-bit support)
- **LD** - Linker
- **Make** - Build automation (optional)
- **QEMU** - Emulator for testing

**See [INSTALLATION.md](docs/INSTALLATION.md) for detailed installation instructions.**

### Quick Install (Windows with Winget):

```powershell
# Install NASM
winget install nasm.nasm

# Install MSYS2 (includes GCC, LD, Make)
winget install MSYS2.MSYS2

# Install QEMU  
winget install SoftwareFreedomConservancy.QEMU
```

After installing MSYS2, open "MSYS2 MINGW64" and run:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils make
```

### 2. Build ValcOS

Once tools are installed, choose one method:

**Option A: Using Make**
```bash
make
```

**Option B: Using PowerShell Script**
```powershell
.\build.ps1
```

**Option C: Manual Commands** (see [INSTALLATION.md](docs/INSTALLATION.md))

### 3. Run ValcOS

```bash
# Using Make
make run

# Or manually
qemu-system-x86_64 -drive format=raw,file=build/ValcOS.img
```

### 4. Try the Shell

Once ValcOS boots, try these commands:
- `help` - Show available commands
- `about` - Display OS information
- `clear` - Clear the screen
- `echo Hello World` - Print text

## Project Files

All the code is ready! Here's what we've created:

```
ValcOS/
├── boot/boot.asm          - Bootloader (512 bytes)
├── kernel/
│   ├── kernel.c           - Main kernel
│   ├── kernel_entry.asm   - Assembly entry point
│   ├── idt.c              - Interrupt handling
│   ├── shell.c            - Command shell
│   └── string.c           - Utility functions
├── drivers/
│   ├── vga.c              - Display driver
│   └── keyboard.c         - Keyboard driver
├── include/               - Header files
├── docs/
│   ├── ARCHITECTURE.md    - Technical documentation
│   └── INSTALLATION.md    - Tool installation guide
├── Makefile               - Build system
├── linker.ld              - Linker script
└── build.ps1              - PowerShell build script
```

## Need Help?

- **Installation Issues**: See [INSTALLATION.md](docs/INSTALLATION.md)
- **Technical Details**: See [ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Build Errors**: Check that all tools are in your PATH

---

**Ready to build your own OS? Install the tools and let's go! 🚀**
