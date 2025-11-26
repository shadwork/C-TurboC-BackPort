# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
if(APPLE)
    project(TurboC-Backport LANGUAGES C CXX OBJCXX)
else()
    project(TurboC-Backport LANGUAGES C CXX)
endif()

add_compile_definitions(WRAPPER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# --- Variables ---

# Define the executable name (Used by the samples subdirectory)
set(PCCORE_NAME pccore)
set(MATRIX_NAME matrix)
set(FIRE_NAME fire)
set(PLASMA_NAME plasma)

# Configure WRAPPER_SOURCES based on the operating system
if(APPLE)
    add_compile_definitions(MACOS)
    # macOS specific sources (Objective-C files)
    set(WRAPPER_SOURCES
        ${CMAKE_SOURCE_DIR}/wrapper/macos_keyboard.m
        ${CMAKE_SOURCE_DIR}/wrapper/macos.m
    )   
    set(WRAPPER_LINK_LIBRARIES
        ${PCCORE_NAME}
        "-framework Cocoa"
        "-framework AppKit"
    )    
    set(WRAPPER_COMPILE_OPTIONS
        -fobjc-arc
        -Wall
        -g
        -fmodules
        -fcxx-modules
    )    
    # Set the C and Objective-C compilers
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_OBJC_COMPILER "clang")    
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
    set(WRAPPER_LINK_LIBRARIES
        ${PCCORE_NAME}
        gdi32
        user32
        kernel32
    )
    set(WRAPPER_COMPILE_OPTIONS
        -mwindows
    )             
    # Set the C and Objective-C compilers
    set(CMAKE_C_COMPILER "gcc")
    message(STATUS "Configuring for Windows (WIN32).")
else()
    # Fallback for unsupported platforms
    message(FATAL_ERROR "Unsupported platform for WRAPPER_SOURCES configuration. Please add support for ${CMAKE_SYSTEM_NAME}.")
endif()

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
add_subdirectory(samples/plasma)