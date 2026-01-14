# ValcOS

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Status](https://img.shields.io/badge/status-active-success.svg)

A simple x86 operating system built from scratch for learning and experimentation.

## Features

✨ **Custom Bootloader** - Loads kernel and switches to protected mode  
🖥️ **VGA Text Mode** - 80x25 color display with scrolling  
⌨️ **Keyboard Driver** - Interrupt-driven PS/2 keyboard input  
💻 **Interactive Shell** - Command-line interface with built-in commands  
🎨 **Color Support** - 16-color text output  

## Prerequisites

- **NASM** - Netwide Assembler for x86 assembly
- **GCC** - GNU Compiler Collection (with 32-bit support)
- **LD** - GNU Linker
- **Make** - Build automation tool
- **QEMU** - x86 emulator for testing

### Installing Prerequisites on Windows

```powershell
# Install NASM
winget install nasm.nasm

# Install MinGW (includes GCC, LD, Make)
winget install -e --id GnuWin32.Make

# Install QEMU
winget install -e --id SoftwareFreedomConservancy.QEMU
```

## Quick Start

### Building ValcOS

```bash
# Clone the repository
cd ValcOS

# Build the OS
make

# Or use the build script
scripts\build.bat
```

This will create `build/ValcOS.img` - a bootable 1.44MB disk image.

### Running in QEMU

```bash
# Run with make
make run

# Or use the run script
scripts\run.bat

# Or manually
qemu-system-x86_64 -drive format=raw,file=build/ValcOS.img
```

## Project Structure

```
ValcOS/
├── boot/              # Bootloader
│   └── boot.asm       # 16-bit bootloader with protected mode switch
├── kernel/            # Kernel source
│   ├── kernel.c       # Main kernel entry point
│   ├── kernel_entry.asm  # Assembly kernel entry
│   ├── idt.c          # Interrupt descriptor table
│   ├── shell.c        # Command shell
│   └── string.c       # String utilities
├── drivers/           # Device drivers
│   ├── vga.c          # VGA text mode driver
│   └── keyboard.c     # PS/2 keyboard driver
├── include/           # Header files
├── build/             # Compiled binaries
├── scripts/           # Build and run scripts
├── docs/              # Documentation
│   └── ARCHITECTURE.md  # Technical documentation
├── Makefile           # Build system
└── linker.ld          # Linker script
```

## Shell Commands

Once ValcOS boots, you can use these commands:

| Command | Description |
|---------|-------------|
| `help` | Display available commands |
| `clear` | Clear the screen |
| `about` | Show OS information |
| `echo <text>` | Print text to screen |

## How It Works

1. **BIOS** loads the bootloader from the first sector into memory at `0x7C00`
2. **Bootloader** loads the kernel from disk and switches to 32-bit protected mode
3. **Kernel** initializes VGA display, sets up interrupt handling, and enables the keyboard
4. **Shell** provides an interactive command-line interface

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed technical documentation.

## Development

### Building Individual Components

```bash
# Clean build artifacts
make clean

# Build only
make all

# Build and run
make run
```

### Debugging

To debug with GDB:

```bash
# Terminal 1: Start QEMU with GDB server
qemu-system-x86_64 -drive format=raw,file=build/ValcOS.img -s -S

# Terminal 2: Connect GDB
gdb
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Roadmap

- [x] Bootloader with protected mode
- [x] VGA text mode driver
- [x] Keyboard input
- [x] Basic shell
- [ ] Timer/PIT driver
- [ ] Memory management (paging, heap)
- [ ] Filesystem support (FAT12)
- [ ] Multitasking
- [ ] System calls
- [ ] User mode

## Contributing

Contributions are welcome! Feel free to:

- Report bugs
- Suggest features
- Submit pull requests
- Improve documentation

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

Built with passion as a learning project to understand operating system fundamentals.

## Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Comprehensive OS development resource
- [Intel x86 Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - Official processor documentation
- [NASM Documentation](https://www.nasm.us/docs.php) - Assembler reference

---

**Made with ❤️ for learning and exploration**
