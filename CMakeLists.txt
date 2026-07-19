cmake_minimum_required(VERSION 3.22)
project(VulkanCalc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    # Windows resource file with icon
    set(RC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
endif()

# MSVC: source files are UTF-8 (fixes Chinese chars in codepage 936)
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/wd4819)
endif()

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# ImGui sources
set(IMGUI_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_vulkan.cpp
)

# Collect sources
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create executable (with icon on Windows)
if(WIN32)
    # Compile .rc file to .res using windres
    find_program(WINDRES windres)
    if(WINDRES)
        set(RES_OUT ${CMAKE_CURRENT_BINARY_DIR}/resources.res)
        add_custom_command(
            OUTPUT ${RES_OUT}
            COMMAND ${WINDRES} -i ${RC_SRC} -o ${RES_OUT} --define _WIN32
            DEPENDS ${RC_SRC}
            COMMENT "Compiling resources"
        )
        add_executable(vulkan_calc WIN32 ${SOURCES} ${IMGUI_SRC} ${RES_OUT})
    else()
        add_executable(vulkan_calc WIN32 ${SOURCES} ${IMGUI_SRC})
    endif()
    # Set icon in the binary itself via linker flag
    set_target_properties(vulkan_calc PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
else()
    add_executable(vulkan_calc ${SOURCES} ${IMGUI_SRC})
endif()

target_include_directories(vulkan_calc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(vulkan_calc PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)
