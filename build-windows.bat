@echo off
REM VulkanCalc Windows Build Script
REM 从 VS2022 开发者命令提示符运行

setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build-windows
set OUTPUT_DIR=F:\vulkan-calc

echo ========================================
echo Building VulkanCalc for Windows
echo Casio fx-991CNX Style Calculator
echo ========================================

REM ---- Vulkan SDK ----
if "%VULKAN_SDK%"=="" (
    if exist "E:\Programming\vulkan\1.4.350.0" (
        set VULKAN_SDK=E:\Programming\vulkan\1.4.350.0
    ) else if exist "C:\VulkanSDK\1.4.350.0" (
        set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
    ) else if exist "C:\VulkanSDK" (
        set VULKAN_SDK=C:\VulkanSDK
    ) else (
        echo [ERROR] Vulkan SDK not found.
        echo   Download: https://vulkan.lunarg.com/sdk/home
        pause & exit /b 1
    )
)
echo [OK] Vulkan SDK: %VULKAN_SDK%

REM ---- vcpkg ----
if "%VCPKG_ROOT%"=="" (
    if exist "E:\Programming\vcpkg" (
        set VCPKG_ROOT=E:\Programming\vcpkg
    ) else if exist "C:\vcpkg" (
        set VCPKG_ROOT=C:\vcpkg
    ) else (
        echo [WARN] vcpkg not found (glfw3/glm may fail)
    )
)
if not "%VCPKG_ROOT%"=="" echo [OK] vcpkg: %VCPKG_ROOT%

REM ---- Build dir ----
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ---- CMake Configure ----
echo [..] Configuring...
if not "%VCPKG_ROOT%"=="" (
    cmake "%PROJECT_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%VULKAN_SDK%" -DVULKAN_SDK="%VULKAN_SDK%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
) else (
    cmake "%PROJECT_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%VULKAN_SDK%" -DVULKAN_SDK="%VULKAN_SDK%"
)
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed
    pause & exit /b 1
)

REM ---- Build ----
echo [..] Building...
cmake --build . --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed
    pause & exit /b 1
)

REM ---- Copy output ----
echo [..] Copying to %OUTPUT_DIR%
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
copy /Y "%BUILD_DIR%\Release\vulkan_calc.exe" "%OUTPUT_DIR%\"
copy /Y "%PROJECT_DIR%\icon\app.ico" "%OUTPUT_DIR%\"
if exist "%VULKAN_SDK%\Bin\vulkan-1.dll" copy /Y "%VULKAN_SDK%\Bin\vulkan-1.dll" "%OUTPUT_DIR%\"

echo ========================================
echo [DONE] Build complete!
echo   EXE:  %OUTPUT_DIR%\vulkan_calc.exe
echo ========================================
pause
