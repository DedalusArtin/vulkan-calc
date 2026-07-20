@echo off
setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
set PROJECT_DIR=%PROJECT_DIR:~0,-1%
set BUILD_DIR=%PROJECT_DIR%\build-windows
set OUTPUT_DIR=F:\vulkan-calc

echo ========================================
echo Building VulkanCalc for Windows
echo ========================================

REM ---- Vulkan SDK ----
if "%VULKAN_SDK%"=="" (
    if exist "E:\Programming\vulkan\1.4.350.0" set VULKAN_SDK=E:\Programming\vulkan\1.4.350.0
    if exist "C:\VulkanSDK\1.4.350.0" set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
    if exist "C:\VulkanSDK" set VULKAN_SDK=C:\VulkanSDK
)
if "%VULKAN_SDK%"=="" echo [ERROR] Vulkan SDK not found & pause & exit /b 1
echo [OK] Vulkan SDK: %VULKAN_SDK%

REM ---- vcpkg ----
if exist "E:\Programming\vcpkg" set VCPKG_ROOT=E:\Programming\vcpkg
if exist "C:\vcpkg" set VCPKG_ROOT=C:\vcpkg
if not "%VCPKG_ROOT%"=="" echo [OK] vcpkg: %VCPKG_ROOT%

REM ---- Build dir ----
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ---- CMake init cache (use forward slashes, avoid batch escaping issues) ----
echo [..] Configuring...
set VK_DIR=%VULKAN_SDK:\=/%
echo set(CMAKE_PREFIX_PATH "%VK_DIR%") > "%BUILD_DIR%\initCache.cmake"
echo set(VULKAN_SDK "%VK_DIR%") >> "%BUILD_DIR%\initCache.cmake"

if not "%VCPKG_ROOT%"=="" (
    set VCPKG_DIR=%VCPKG_ROOT:\=/%
    rem Use escaped parens to avoid batch if-block ending
    echo set(CMAKE_TOOLCHAIN_FILE "%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake"^) >> "%BUILD_DIR%\initCache.cmake"
)

cmake -C "%BUILD_DIR%\initCache.cmake" -G "Visual Studio 17 2022" -A x64 "%PROJECT_DIR%"
if ERRORLEVEL 1 echo [ERROR] Config failed & pause & exit /b 1

REM ---- Build ----
echo [..] Building...
cmake --build . --config Release --parallel
if ERRORLEVEL 1 echo [ERROR] Build failed & pause & exit /b 1

REM ---- Copy ----
echo [..] Copying...
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
copy /Y "%BUILD_DIR%\Release\vulkan_calc.exe" "%OUTPUT_DIR%\"
if exist "%VULKAN_SDK%\Bin\vulkan-1.dll" copy /Y "%VULKAN_SDK%\Bin\vulkan-1.dll" "%OUTPUT_DIR%\"
REM Copy glfw3 runtime DLL
if exist "%VCPKG_ROOT%\installed\x64-windows\bin\glfw3.dll" copy /Y "%VCPKG_ROOT%\installed\x64-windows\bin\glfw3.dll" "%OUTPUT_DIR%\"
echo [DONE] %OUTPUT_DIR%\vulkan_calc.exe
pause
