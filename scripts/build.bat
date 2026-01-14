@echo off
echo Building ValcOS...
echo.

REM Check for NASM
where nasm >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: NASM not found! Please install NASM.
    exit /b 1
)

REM Check for GCC
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC not found! Please install MinGW or similar.
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Run make
make
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo OS image created at: build\ValcOS.img
