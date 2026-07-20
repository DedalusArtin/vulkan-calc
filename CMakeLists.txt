cmake_minimum_required(VERSION 3.22)
project(VulkanCalc VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

# ============================================================
# Platform detection
# ============================================================
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)

    # Windows resource file with icon
    set(RC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
endif()

# ============================================================
# MSVC-specific options (fixes Chinese chars)
# ============================================================
if(MSVC)
    add_compile_options(/utf-8 /EHsc)
    add_compile_options(/wd4819)
endif()

# ============================================================
# Find Vulkan (with explicit path fallback for Windows)
# ============================================================
find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
    # Fallback: try VULKAN_SDK env var or explicit paths
    if(DEFINED ENV{VULKAN_SDK})
        set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
        set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
        set(Vulkan_FOUND TRUE)
        message(STATUS "Vulkan SDK found via VULKAN_SDK: $ENV{VULKAN_SDK}")
    else()
        message(FATAL_ERROR "Vulkan SDK not found. Set VULKAN_SDK env var.")
    endif()
endif()

# ============================================================
# Find glfw3
# ============================================================
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # Try common vcpkg paths on Windows
    if(WIN32 AND DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
        find_package(glfw3 REQUIRED)
    elseif(WIN32)
        # Try default vcpkg location
        if(EXISTS "C:/vcpkg/installed/x64-windows/share")
            set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
            find_package(glfw3 REQUIRED)
        elseif(EXISTS "$ENV{LOCALAPPDATA}/vcpkg/installed/x64-windows/share")
            set(CMAKE_PREFIX_PATH "$ENV{LOCALAPPDATA}/vcpkg/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
            find_package(glfw3 REQUIRED)
        else()
            find_package(glfw3 REQUIRED)
        endif()
    else()
        find_package(glfw3 REQUIRED)
    endif()
endif()

# ============================================================
# Find glm
# ============================================================
find_package(glm QUIET)
if(NOT glm_FOUND)
    # Try vcpkg paths
    if(WIN32 AND DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
        find_package(glm REQUIRED)
    elseif(WIN32)
        if(EXISTS "C:/vcpkg/installed/x64-windows/share")
            set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
            find_package(glm REQUIRED)
        elseif(EXISTS "$ENV{LOCALAPPDATA}/vcpkg/installed/x64-windows/share")
            set(CMAKE_PREFIX_PATH "$ENV{LOCALAPPDATA}/vcpkg/installed/x64-windows/share" ${CMAKE_PREFIX_PATH})
            find_package(glm REQUIRED)
        else()
            find_package(glm REQUIRED)
        endif()
    else()
        find_package(glm REQUIRED)
    endif()
endif()

# ============================================================
# ImGui sources
# ============================================================
set(IMGUI_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_vulkan.cpp
)

# ============================================================
# Project sources (now split into multiple files)
# ============================================================
set(PROJECT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/calc_logic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math_engine.cpp
)

set(PROJECT_HDR
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/locale.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/calc_logic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math_engine.hpp
)

# ============================================================
# Create executable
# ============================================================
if(WIN32)
    # On MSVC, prefer rc.exe (MSVC resource compiler) over MinGW windres
    if(MSVC)
        find_program(RC_COMPILER rc)
    endif()
    if(NOT RC_COMPILER)
        find_program(WINDRES windres)
    endif()

    if(RC_COMPILER)
        # MSVC-style rc.exe (preferred for MSVC builds)
        set(RES_OUT ${CMAKE_CURRENT_BINARY_DIR}/resources.res)
        add_custom_command(
            OUTPUT ${RES_OUT}
            COMMAND ${RC_COMPILER} /fo ${RES_OUT} ${RC_SRC}
            DEPENDS ${RC_SRC}
            COMMENT "Compiling resources (rc.exe)"
        )
        add_executable(vulkan_calc ${PROJECT_SRC} ${PROJECT_HDR} ${IMGUI_SRC} ${RES_OUT})
    elseif(WINDRES)
        # MinGW-style windres
        set(RES_OUT ${CMAKE_CURRENT_BINARY_DIR}/resources.res)
        add_custom_command(
            OUTPUT ${RES_OUT}
            COMMAND ${WINDRES} -i ${RC_SRC} -o ${RES_OUT}
            DEPENDS ${RC_SRC}
            COMMENT "Compiling resources (windres)"
        )
        add_executable(vulkan_calc ${PROJECT_SRC} ${PROJECT_HDR} ${IMGUI_SRC} ${RES_OUT})
    else()
        # No resource compiler found, build without icon
        message(WARNING "No resource compiler (windres or rc) found. Building without icon.")
        add_executable(vulkan_calc ${PROJECT_SRC} ${PROJECT_HDR} ${IMGUI_SRC})
    endif()

    # Console subsystem (needed by GLFW's main())
    # Explicitly set CONSOLE — do NOT use WIN32 subsystem
    set_target_properties(vulkan_calc PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    if(MSVC)
        target_link_options(vulkan_calc PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
else()
    add_executable(vulkan_calc ${PROJECT_SRC} ${PROJECT_HDR} ${IMGUI_SRC})
endif()

# ============================================================
# Include directories
# ============================================================
target_include_directories(vulkan_calc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(vulkan_calc PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)
