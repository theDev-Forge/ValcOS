@echo off
echo Running ValcOS in QEMU...
echo.

if not exist build\ValcOS.img (
    echo ERROR: OS image not found! Please build first.
    echo Run: scripts\build.bat
    exit /b 1
)

qemu-system-x86_64 -drive format=raw,file=build\ValcOS.img
