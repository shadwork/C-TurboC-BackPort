# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(TurboC LANGUAGES C CXX OBJCXX)

# --- Variables ---

# Configure WRAPPER_SOURCES based on the operating system
if(APPLE)
    # macOS specific sources (Objective-C files)
    set(WRAPPER_SOURCES
        ${CMAKE_SOURCE_DIR}/wrapper/macos_keyboard.m
        ${CMAKE_SOURCE_DIR}/wrapper/macos.m
    )
    message(STATUS "Configuring for macOS (APPLE).")
elseif(UNIX)
    # Linux/BSD specific sources (C files). Note: APPLE is checked first.
    set(WRAPPER_SOURCES
        ${CMAKE_SOURCE_DIR}/wrapper/linux_keyboard.c
        ${CMAKE_SOURCE_DIR}/wrapper/linux.c
    )
    message(STATUS "Configuring for Linux/UNIX.")
elseif(WIN32)
    # Windows specific sources (C files)
    set(WRAPPER_SOURCES
        ${CMAKE_SOURCE_DIR}/wrapper/windows_keyboard.c
        ${CMAKE_SOURCE_DIR}/wrapper/windows.c
    )
    message(STATUS "Configuring for Windows (WIN32).")
else()
    # Fallback for unsupported platforms
    message(FATAL_ERROR "Unsupported platform for WRAPPER_SOURCES configuration. Please add support for ${CMAKE_SYSTEM_NAME}.")
endif()


# Define the executable name (Used by the samples subdirectory)
set(PCCORE_NAME pccore)
set(MATRIX_NAME matrix)
set(FIRE_NAME fire)

# --- Compiler Configuration ---

# Set the C and Objective-C compilers
set(CMAKE_C_COMPILER "clang")
set(CMAKE_OBJC_COMPILER "clang")

# Define common include paths for all targets
set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/pccore
    ${CMAKE_SOURCE_DIR}/turboc
    ${CMAKE_SOURCE_DIR}/wrapper 
)

# 1. Define sources and build the pccore library
add_subdirectory(wrapper)

# 2. Build the samples
add_subdirectory(samples/matrix)
add_subdirectory(samples/fire)