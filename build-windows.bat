@echo off
REM Windows Build Script for VulkanCalc 2.0 (Casio Calculator)
REM Run from a Developer Command Prompt for VS 2022
REM Requires: VS 2022, Vulkan SDK, CMake 3.22+, vcpkg (for glfw3/glm)

setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build-windows
set OUTPUT_DIR=%PROJECT_DIR%dist

echo ========================================
echo Building VulkanCalc for Windows
echo Casio fx-991CNX Style Calculator
echo ========================================

REM Check Vulkan SDK
if "%VULKAN_SDK%"=="" (
    echo [ERROR] Vulkan SDK not found.
    echo   Download from: https://vulkan.lunarg.com/sdk/home
    pause
    exit /b 1
)
echo [OK] Vulkan SDK: %VULKAN_SDK%

REM Check vcpkg
if "%VCPKG_ROOT%"=="" (
    if exist "C:\vcpkg" (
        set VCPKG_ROOT=C:\vcpkg
    ) else if exist "%LOCALAPPDATA%\vcpkg" (
        set VCPKG_ROOT=%LOCALAPPDATA%\vcpkg
    ) else (
        echo [WARN] vcpkg not found. Installing glfw3 and glm may fail.
        echo   Install vcpkg from: https://github.com/microsoft/vcpkg
    )
)
if not "%VCPKG_ROOT%"=="" (
    echo [OK] vcpkg: %VCPKG_ROOT%
)

REM Create build dir
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Configure with CMake (console subsystem for GLFW debug output)
echo [..] Configuring with CMake...
set CMAKE_ARGS=-G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%VULKAN_SDK%" ^
    -DVULKAN_SDK="%VULKAN_SDK%"

if not "%VCPKG_ROOT%"=="" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

cmake "%PROJECT_DIR%" %CMAKE_ARGS%

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

REM Copy required DLLs (from Vulkan SDK)
if exist "%VULKAN_SDK%\bin\vulkan-1.dll" (
    copy /Y "%VULKAN_SDK%\bin\vulkan-1.dll" "%OUTPUT_DIR%\"
)

echo ========================================
echo [DONE] Build complete!
echo   EXE:  %OUTPUT_DIR%\vulkan_calc.exe
echo ========================================
pause
