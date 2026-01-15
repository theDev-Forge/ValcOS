#!/bin/bash
#
# Build script for ValcOS userspace programs
#
# Requirements:
# - i686-elf-gcc cross-compiler
# - i686-elf-ld linker
#

# Compiler settings
CC=i686-elf-gcc
LD=i686-elf-ld
CFLAGS="-m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector"
LDFLAGS="-m elf_i386 -T userspace/linker.ld"

echo "Building hello.c..."

# Compile
$CC $CFLAGS -c userspace/hello.c -o build/hello.o

# Link
$LD $LDFLAGS build/hello.o -o build/hello.elf

echo "Built: build/hello.elf"
echo ""
echo "To add to filesystem:"
echo "1. Copy build/hello.elf to rootfs/hello.elf"
echo "2. Run: make run"
echo "3. In ValcOS shell: exec /hello.elf"
