@echo off
echo Running ValcOS in QEMU...
echo.

if not exist build\ValcOS.img (
    echo ERROR: OS image not found! Please build first.
    echo Run: scripts\build.bat
    exit /b 1
)

"C:\Program Files\qemu\qemu-system-x86_64.exe" -fda build\ValcOS.img -boot a
