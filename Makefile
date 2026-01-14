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

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_ENTRY_SRC = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_SRC = $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/shell.c $(KERNEL_DIR)/string.c $(KERNEL_DIR)/memory.c
DRIVER_SRC = $(DRIVERS_DIR)/vga.c $(DRIVERS_DIR)/keyboard.c $(DRIVERS_DIR)/timer.c

# Object files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
KERNEL_OBJ = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRC))
DRIVER_OBJ = $(patsubst $(DRIVERS_DIR)/%.c, $(BUILD_DIR)/%.o, $(DRIVER_SRC))

# Output
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/ValcOS.img

# Default target
all: $(OS_IMAGE)

# Create OS image
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "Creating OS image..."
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	@echo "Padding image to 1.44MB..."
	@truncate -s 1474560 $(OS_IMAGE) 2>/dev/null || dd if=/dev/zero of=$(OS_IMAGE) bs=1474560 count=1 conv=notrunc 2>/dev/null
	@echo "Build complete: $(OS_IMAGE)"

# Build bootloader
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	@echo "Assembling bootloader..."
	$(ASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

# Build kernel binary (link then extract)
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(DRIVER_OBJ)
	@echo "Linking kernel..."
	$(LD) $(LD_FLAGS) -o $(KERNEL_ELF) $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) $(DRIVER_OBJ)
	@echo "Extracting binary..."
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

# Build kernel entry
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC) | $(BUILD_DIR)
	@echo "Assembling kernel entry..."
	$(ASM) $(ASM_FLAGS) $(KERNEL_ENTRY_SRC) -o $(KERNEL_ENTRY_OBJ)

# Build kernel C files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Build driver C files
$(BUILD_DIR)/%.o: $(DRIVERS_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(C_FLAGS) -I$(INCLUDE_DIR) $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

QEMU = "C:\Program Files\qemu\qemu-system-x86_64.exe"

# Run in QEMU
run: $(OS_IMAGE)
	$(QEMU) -fda $(OS_IMAGE) -boot a

.PHONY: all clean run
