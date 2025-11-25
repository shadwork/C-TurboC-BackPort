# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(TurboC-Backport LANGUAGES C CXX OBJCXX)

# --- Variables ---

# Define the executable name
set(TARGET_NAME matrix)

# Define source files for the pccore library
set(PCCORE_SOURCES
    pccore/pccore.c
    pccore/cga.c
    pccore/cgafont.c
    turboc/dos.c
    turboc/bios.c
    turboc/int10.c
    turboc/conio.c
)

# Define source files for the executable
set(MATRIX_SOURCES
    samples/matrix.c
    wrapper/macos_keyboard.m
    wrapper/macos.m
)

# --- Compiler Configuration ---

# Set the C compiler (equivalent to CC = clang)
# CMake will typically find clang if it's the system default or specified via env var.
set(CMAKE_C_COMPILER "clang")
set(CMAKE_OBJC_COMPILER "clang")

# --- Include Directories ---

# Define common include paths for all targets
set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/pccore
    ${CMAKE_SOURCE_DIR}/turboc
    ${CMAKE_SOURCE_DIR}/samples
)

# --- Library Builds ---

## 1. pccore Library
add_library(pccore STATIC ${PCCORE_SOURCES})

# Make sure the pccore library can see its own headers and the turboc headers if needed
target_include_directories(pccore PUBLIC ${COMMON_INCLUDES})

# --- Executable Build ---

# Add the executable target
add_executable(${TARGET_NAME} ${MATRIX_SOURCES})

# Link the libraries to the executable
target_link_libraries(${TARGET_NAME} PRIVATE
    pccore
    "-framework Cocoa"
    "-framework AppKit"
)

# Set common include directories for the executable
target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDES})

# --- Compilation Flags (CFLAGS) ---

# Apply general compiler flags (C, CXX, Objective-C/C++ sources)
target_compile_options(${TARGET_NAME} PRIVATE
    -fobjc-arc
    -Wall
    -g
    -fmodules
    -fcxx-modules
)

# Define the preprocessor macro (DWRAPPER)
target_compile_definitions(${TARGET_NAME} PRIVATE
    WRAPPER
)

# --- Linking Flags (LDFLAGS) ---

# Link options for the executable (mostly redundant now that link libraries handles frameworks)
target_link_options(${TARGET_NAME} PRIVATE
    -fmodules
    -fcxx-modules
)