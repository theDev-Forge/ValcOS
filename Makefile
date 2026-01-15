# ValcOS Makefile

# Assembler and Compiler
ASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy

# Flags
ASM_FLAGS = -f elf32
C_FLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -c -Wall -Wextra
LD_FLAGS = -m i386pe --oformat pe-i386 -e _kernel_entry -Ttext 0x10000

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
INCLUDE_DIR = include
APPS_DIR = apps

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_ENTRY_SRC = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_SRC = $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/shell.c $(KERNEL_DIR)/string.c $(KERNEL_DIR)/memory.c $(KERNEL_DIR)/slab.c $(KERNEL_DIR)/printk.c $(KERNEL_DIR)/ktimer.c $(KERNEL_DIR)/workqueue.c $(KERNEL_DIR)/signal.c $(KERNEL_DIR)/netdevice.c $(KERNEL_DIR)/skbuff.c $(KERNEL_DIR)/socket.c $(KERNEL_DIR)/vfs.c $(KERNEL_DIR)/blkdev.c $(KERNEL_DIR)/device.c $(KERNEL_DIR)/elf.c $(KERNEL_DIR)/process.c $(KERNEL_DIR)/gdt.c $(KERNEL_DIR)/tss.c $(KERNEL_DIR)/syscall.c $(KERNEL_DIR)/pmm.c $(KERNEL_DIR)/vmm.c fs/fat12.c
DRIVER_SRC = $(DRIVERS_DIR)/vga.c $(DRIVERS_DIR)/keyboard.c $(KERNEL_DIR)/timer.c $(DRIVERS_DIR)/rtc.c drivers/net/loopback.c

# Object files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
PROCESS_ASM_OBJ = $(BUILD_DIR)/process_asm.o
KERNEL_OBJS = $(BUILD_DIR)/kernel.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/string.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/slab.o $(BUILD_DIR)/printk.o $(BUILD_DIR)/ktimer.o $(BUILD_DIR)/workqueue.o $(BUILD_DIR)/signal.o $(BUILD_DIR)/netdevice.o $(BUILD_DIR)/skbuff.o $(BUILD_DIR)/socket.o $(BUILD_DIR)/vfs.o $(BUILD_DIR)/blkdev.o $(BUILD_DIR)/device.o $(BUILD_DIR)/elf.o $(BUILD_DIR)/fat12.o $(BUILD_DIR)/process.o $(PROCESS_ASM_OBJ) $(BUILD_DIR)/gdt.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/pmm.o $(BUILD_DIR)/vmm.o
DRIVER_OBJS = $(BUILD_DIR)/vga.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/vga_gfx.o $(BUILD_DIR)/rtc.o $(BUILD_DIR)/loopback.o

# Output
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/ValcOS.img

# Apps
APPS_SRC = $(APPS_DIR)/hello.asm $(APPS_DIR)/input.asm
APPS_BIN = $(BUILD_DIR)/hello.bin $(BUILD_DIR)/input.bin

# Default target
all: $(OS_IMAGE)

# Create OS image
# Create OS image
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) $(APPS_BIN)
	@echo "Creating FAT12 Image..."
	@mkdir -p rootfs
	@cp $(APPS_BIN) rootfs/
	@$(PYTHON) scripts/make_fat12.py
	@echo "Build complete: $(OS_IMAGE)"

# Build bootloader
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	@echo "Assembling bootloader..."
	$(ASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# Build kernel binary
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJS) $(DRIVER_OBJS)
	@echo "Linking kernel..."
	$(LD) $(LD_FLAGS) -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJS) $(DRIVER_OBJS)
	@echo "Extracting binary..."
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

# Assemble kernel entry
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	@echo "Assembling kernel entry..."
	nasm -f elf32 $< -o $@

# Assemble process switching
$(PROCESS_ASM_OBJ): $(KERNEL_DIR)/process.asm | $(BUILD_DIR)
	@echo "Assembling $<..."
	nasm -f elf32 $< -o $@

# Assemble usermode switching
$(BUILD_DIR)/usermode.o: $(KERNEL_DIR)/usermode.asm | $(BUILD_DIR)
	@echo "Assembling $<..."
	nasm -f elf32 $< -o $@

# Build Apps
$(BUILD_DIR)/%.bin: $(APPS_DIR)/%.asm | $(BUILD_DIR)
	@echo "Assembling App $<..."
	nasm -f bin $< -o $@

# Build kernel C files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Build fs C files
$(BUILD_DIR)/%.o: fs/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Build driver C files
$(BUILD_DIR)/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Build drivers/net C files
$(BUILD_DIR)/%.o: drivers/net/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf rootfs
	@echo "Clean complete."

QEMU = "C:\Program Files\qemu\qemu-system-x86_64.exe"
PYTHON = "C:\Users\Administrator\AppData\Local\Programs\Python\Python312\python.exe"

# Run in QEMU
run: $(OS_IMAGE)
	$(QEMU) -fda $(OS_IMAGE) -boot a

.PHONY: all clean run
