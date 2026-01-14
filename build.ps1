# ValcOS Build Script (PowerShell)
# This script builds ValcOS without requiring Make

Write-Host "Building ValcOS..." -ForegroundColor Cyan
Write-Host ""

# Check for required tools
$tools = @{
    "nasm" = "NASM (Netwide Assembler)"
    "gcc"  = "GCC (GNU Compiler Collection)"
    "ld"   = "LD (GNU Linker)"
}

$missingTools = @()
foreach ($tool in $tools.Keys) {
    $found = Get-Command $tool -ErrorAction SilentlyContinue
    if (-not $found) {
        $missingTools += $tools[$tool]
        Write-Host "ERROR: $($tools[$tool]) not found!" -ForegroundColor Red
    }
    else {
        Write-Host "✓ Found $($tools[$tool])" -ForegroundColor Green
    }
}

if ($missingTools.Count -gt 0) {
    Write-Host ""
    Write-Host "Please install the missing tools:" -ForegroundColor Yellow
    foreach ($tool in $missingTools) {
        Write-Host "  - $tool" -ForegroundColor Yellow
    }
    exit 1
}

Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Host "Created build directory" -ForegroundColor Green
}

# Build bootloader
Write-Host "Assembling bootloader..." -ForegroundColor Cyan
nasm -f bin boot/boot.asm -o build/boot.bin
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Bootloader assembly failed!" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Bootloader assembled" -ForegroundColor Green

# Build kernel entry
Write-Host "Assembling kernel entry..." -ForegroundColor Cyan
nasm -f elf32 kernel/kernel_entry.asm -o build/kernel_entry.o
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Kernel entry assembly failed!" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Kernel entry assembled" -ForegroundColor Green

# Compile kernel C files
$cFiles = @(
    "kernel/kernel.c",
    "kernel/idt.c",
    "kernel/shell.c",
    "kernel/string.c",
    "drivers/vga.c",
    "drivers/keyboard.c"
)

$cFlags = "-m32", "-ffreestanding", "-nostdlib", "-fno-pie", "-fno-stack-protector", "-c", "-Wall", "-Wextra", "-Iinclude"

foreach ($file in $cFiles) {
    $basename = [System.IO.Path]::GetFileNameWithoutExtension($file)
    $output = "build/$basename.o"
    
    Write-Host "Compiling $file..." -ForegroundColor Cyan
    gcc @cFlags $file -o $output
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Compilation of $file failed!" -ForegroundColor Red
        exit 1
    }
    Write-Host "✓ Compiled $basename" -ForegroundColor Green
}

# Link kernel
Write-Host "Linking kernel..." -ForegroundColor Cyan
$objectFiles = @(
    "build/kernel_entry.o",
    "build/kernel.o",
    "build/idt.o",
    "build/shell.o",
    "build/string.o",
    "build/vga.o",
    "build/keyboard.o"
)

ld -m elf_i386 -T linker.ld -o build/kernel.bin @objectFiles
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Kernel linking failed!" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Kernel linked" -ForegroundColor Green

# Create OS image
Write-Host "Creating OS image..." -ForegroundColor Cyan
Get-Content -Path "build/boot.bin", "build/kernel.bin" -Encoding Byte -Raw | Set-Content -Path "build/ValcOS.img" -Encoding Byte

# Pad to 1.44MB (1474560 bytes)
$img = [System.IO.File]::Open("build/ValcOS.img", [System.IO.FileMode]::Open)
$img.SetLength(1474560)
$img.Close()

Write-Host "✓ OS image created" -ForegroundColor Green
Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "OS image: build/ValcOS.img" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run: & 'C:\Program Files\qemu\qemu-system-x86_64.exe' -fda build/ValcOS.img -boot a" -ForegroundColor Yellow
