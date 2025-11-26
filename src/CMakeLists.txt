# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(TurboC LANGUAGES C CXX OBJCXX)

# --- Variables ---

set(WRAPPER_SOURCES
    ${CMAKE_SOURCE_DIR}/wrapper/macos_keyboard.m
    ${CMAKE_SOURCE_DIR}/wrapper/macos.m
)

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

# 2. Build the final executable
add_subdirectory(samples)

add_subdirectory(samples/fire)