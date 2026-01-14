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
# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_ENTRY_SRC = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_SRC = $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/shell.c $(KERNEL_DIR)/string.c $(KERNEL_DIR)/memory.c $(KERNEL_DIR)/process.c fs/fat12.c
DRIVER_SRC = $(DRIVERS_DIR)/vga.c $(DRIVERS_DIR)/keyboard.c $(DRIVERS_DIR)/timer.c

# Object files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
PROCESS_ASM_OBJ = $(BUILD_DIR)/process_asm.o
KERNEL_OBJ = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRC))
KERNEL_OBJ := $(KERNEL_OBJ:fs/%.o=$(BUILD_DIR)/%.o) # Fix path for fs/fat12.c
# Actually simpler: just let the pattern match handle it if I use VPATH or explicit rules.
# But KERNEL_OBJ definition above assumes $(KERNEL_DIR)/%.c -> $(BUILD_DIR)/%.o
# This fails for fs/fat12.c.
# Let's fix KERNEL_OBJ and DRIVER_OBJ definitions properly.

# Explicit object lists
KERNEL_OBJS = $(BUILD_DIR)/kernel.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/string.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/fat12.o $(BUILD_DIR)/process.o $(PROCESS_ASM_OBJ)
DRIVER_OBJS = $(BUILD_DIR)/vga.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/timer.o

# Output
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/ValcOS.img

# Default target
all: $(OS_IMAGE)

# Create OS image
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "Creating FAT12 Image..."
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

# Assemble process switcher
$(PROCESS_ASM_OBJ): $(KERNEL_DIR)/process.asm
	@echo "Assembling process context switch..."
	nasm -f elf32 $< -o $@

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

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

QEMU = "C:\Program Files\qemu\qemu-system-x86_64.exe"
PYTHON = "C:\Users\Administrator\AppData\Local\Programs\Python\Python312\python.exe"

# Run in QEMU
run: $(OS_IMAGE)
	$(QEMU) -fda $(OS_IMAGE) -boot a

.PHONY: all clean run
