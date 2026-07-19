@echo off
REM Windows Build Script for VulkanCalc
REM Run from a Developer Command Prompt for VS 2022
REM Requires: VS 2022, Vulkan SDK, CMake 3.22+

setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build-windows
set OUTPUT_DIR=F:\VulkanCalc

echo ========================================
echo Building VulkanCalc for Windows
echo ========================================

REM Check Vulkan SDK
if "%VULKAN_SDK%"=="" (
    echo [ERROR] Vulkan SDK not found.
    echo   Download from: https://vulkan.lunarg.com/sdk/home
    echo   Then restart this script.
    pause
    exit /b 1
)
echo [OK] Vulkan SDK: %VULKAN_SDK%

REM Create build dir
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure
echo [..] Configuring with CMake...
cmake "%PROJECT_DIR%" -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%VULKAN_SDK%" ^
    -DVULKAN_SDK="%VULKAN_SDK%" ^
    -DCMAKE_INSTALL_PREFIX="%OUTPUT_DIR%"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed. See above.
    pause
    exit /b 1
)

REM Build
echo [..] Building...
cmake --build . --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed. See above.
    pause
    exit /b 1
)

REM Copy to output
echo [..] Copying to %OUTPUT_DIR%
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
copy /Y "%BUILD_DIR%\Release\vulkan_calc.exe" "%OUTPUT_DIR%\"
copy /Y "%PROJECT_DIR%\icon\app.ico" "%OUTPUT_DIR%\"

echo ========================================
echo [DONE] Build complete!
echo   EXE:  %OUTPUT_DIR%\vulkan_calc.exe
echo   Icon: %OUTPUT_DIR%\app.ico
echo ========================================
pause
