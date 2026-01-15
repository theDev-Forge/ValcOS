#!/bin/bash
#
# Automated BusyBox build script for ValcOS
# This script automates the entire process
#

set -e

VALCOS_DIR="$(pwd)"
BUILD_DIR="$HOME/valcos-build"
NEWLIB_VERSION="4.3.0"
BUSYBOX_VERSION="1.36.1"

echo "=== ValcOS BusyBox Builder ==="
echo "This script will:"
echo "  1. Download and build newlib"
echo "  2. Download and build BusyBox"
echo "  3. Integrate BusyBox into ValcOS"
echo ""
read -p "Continue? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ===== Part 1: Build newlib =====
echo ""
echo "=== Building newlib ==="

if [ ! -f "newlib-$NEWLIB_VERSION.tar.gz" ]; then
    echo "Downloading newlib..."
    wget "ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VERSION.tar.gz"
fi

if [ ! -d "newlib-$NEWLIB_VERSION" ]; then
    echo "Extracting newlib..."
    tar xzf "newlib-$NEWLIB_VERSION.tar.gz"
fi

cd "newlib-$NEWLIB_VERSION"
mkdir -p build-valcos
cd build-valcos

if [ ! -f "Makefile" ]; then
    echo "Configuring newlib..."
    ../configure \
        --target=i686-elf \
        --prefix=/usr/local/i686-elf \
        --disable-multilib \
        --disable-newlib-supplied-syscalls \
        --enable-newlib-reent-small \
        --enable-newlib-io-long-long \
        --enable-newlib-io-c99-formats
fi

echo "Building newlib (this may take 5-10 minutes)..."
make all -j$(nproc)

echo "Installing newlib (requires sudo)..."
sudo make install

# Build syscall stubs
echo "Building ValcOS syscall stubs..."
cd /usr/local/i686-elf/lib/
sudo cp "$VALCOS_DIR/userspace/newlib_stubs.c" .
sudo i686-elf-gcc -c newlib_stubs.c -o newlib_stubs.o
sudo i686-elf-ar rcs libvalcos.a newlib_stubs.o

echo "✅ newlib built and installed!"

# ===== Part 2: Build BusyBox =====
echo ""
echo "=== Building BusyBox ==="

cd "$BUILD_DIR"

if [ ! -f "busybox-$BUSYBOX_VERSION.tar.bz2" ]; then
    echo "Downloading BusyBox..."
    wget "https://busybox.net/downloads/busybox-$BUSYBOX_VERSION.tar.bz2"
fi

if [ ! -d "busybox-$BUSYBOX_VERSION" ]; then
    echo "Extracting BusyBox..."
    tar xjf "busybox-$BUSYBOX_VERSION.tar.bz2"
fi

cd "busybox-$BUSYBOX_VERSION"

# Copy our config if it exists
if [ -f "$VALCOS_DIR/busybox.config" ]; then
    echo "Using ValcOS BusyBox configuration..."
    cp "$VALCOS_DIR/busybox.config" .config
else
    echo "Creating minimal BusyBox configuration..."
    make allnoconfig
    # Enable minimal set
    scripts/config --enable CONFIG_STATIC
    scripts/config --enable CONFIG_FEATURE_PREFER_APPLETS
    # Core utilities
    scripts/config --enable CONFIG_LS
    scripts/config --enable CONFIG_CAT
    scripts/config --enable CONFIG_ECHO
    scripts/config --enable CONFIG_CP
    scripts/config --enable CONFIG_MV
    scripts/config --enable CONFIG_RM
    scripts/config --enable CONFIG_MKDIR
    # Shell
    scripts/config --enable CONFIG_ASH
    scripts/config --enable CONFIG_FEATURE_SH_IS_ASH
fi

echo "Building BusyBox..."
make clean
make -j$(nproc) \
    CROSS_COMPILE=i686-elf- \
    EXTRA_CFLAGS="-m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -O2" \
    EXTRA_LDFLAGS="-static -nostdlib -T $VALCOS_DIR/userspace/linker.ld" \
    EXTRA_LDLIBS="/usr/local/i686-elf/lib/libc.a /usr/local/i686-elf/lib/libvalcos.a"

echo "Stripping BusyBox..."
i686-elf-strip busybox

echo "✅ BusyBox built! Size: $(du -h busybox | cut -f1)"

# ===== Part 3: Integrate with ValcOS =====
echo ""
echo "=== Integrating BusyBox with ValcOS ==="

cd "$VALCOS_DIR"
mkdir -p rootfs/bin

echo "Copying BusyBox..."
cp "$BUILD_DIR/busybox-$BUSYBOX_VERSION/busybox" rootfs/bin/

echo "Creating symlinks..."
cd rootfs/bin
./busybox --install -s .

echo "✅ BusyBox integrated!"

# ===== Summary =====
echo ""
echo "=== Build Complete! ==="
echo ""
echo "BusyBox has been built and integrated into ValcOS."
echo ""
echo "Next steps:"
echo "  1. Rebuild ValcOS: make clean && make all"
echo "  2. Run ValcOS: make run"
echo "  3. Test BusyBox: exec /bin/ls"
echo ""
echo "Available commands in /bin:"
ls -1 "$VALCOS_DIR/rootfs/bin" | head -20
echo "... and more!"
echo ""
echo "Happy hacking! 🎉"
