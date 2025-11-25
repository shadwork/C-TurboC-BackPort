cmake_minimum_required(VERSION 3.10)

# --- NEW CONFIGURATION FOR GCC ENFORCEMENT ---
# Enforce GCC compiler on Windows and Linux as requested.
if (NOT APPLE)
    message(STATUS "Enforcing GCC/G++ compilers for non-Apple platform.")
    # Set C compiler to gcc
    set(CMAKE_C_COMPILER "gcc")
    # Set CXX compiler to g++ (good practice even if CXX sources aren't currently used)
    set(CMAKE_CXX_COMPILER "g++")
endif()
# --- END OF NEW CONFIGURATION FOR GCC ENFORCEMENT ---

# Define the project and enable C and CXX.
project(AppRunner C CXX) 

# Set C standard for all C source files
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------------
# 1. Collect Common Sources
# --------------------------------------------------------------------------------

# Use file(GLOB) to find all C files in the specified directories.
file(GLOB PCCORE_SOURCES "pccore/*.c")
file(GLOB TURBOC_SOURCES "turboc/*.c")

# Define the common sources for the executable
# samples/matrix.c has been REMOVED from this set.
set(COMMON_SOURCES
    ${PCCORE_SOURCES}
    ${TURBOC_SOURCES}
)

# --- NEW SOURCE SET FOR SAMPLES ---
# Define a separate source set for samples/matrix.c
set(SAMPLE_SOURCES
    samples/matrix.c
)
# --- END NEW SOURCE SET FOR SAMPLES ---

# --------------------------------------------------------------------------------
# 2. Create Executable Target and set common properties
# --------------------------------------------------------------------------------

# The executable now includes both COMMON_SOURCES and the new SAMPLE_SOURCES
add_executable(app_runner ${COMMON_SOURCES} ${SAMPLE_SOURCES})

# Add all relevant directories to the include path for header files (.h)
target_include_directories(app_runner PUBLIC
    pccore
    turboc
    wrapper
    samples
)

# Apply the required compile definition (-DWRAPPER) to all platforms
target_compile_definitions(app_runner PUBLIC
    WRAPPER
)

# --------------------------------------------------------------------------------
# 3. Add Platform-Specific Sources and Linker Flags
# --------------------------------------------------------------------------------

if (APPLE)
    # macOS (Objective-C source files)
    message(STATUS 
"Configuring for macOS (Objective-C)")

    # CRITICAL: Enable Objective-C language support only when compiling on macOS
    enable_language(OBJC)
    
    # Add the Objective-C sources (.m)
    target_sources(app_runner PRIVATE
        wrapper/macos.m
        wrapper/macos_keyboard.m
    )
    
    # Add Objective-C specific compiler options
    # -fmodules is required to handle Cocoa/AppKit framework headers cleanly
    target_compile_options(app_runner PRIVATE
        "$<$<COMPILE_LANGUAGE:OBJC>:-fmodules>"
  
    )

    # Link required frameworks (LDFLAGS = -framework Cocoa -framework AppKit)
    target_link_libraries(app_runner PRIVATE
        "-framework Cocoa"
        "-framework AppKit"
    )

elseif (WIN32)
    # Windows (C source files)
    message(STATUS "Configuring for Windows (using GCC/MinGW)")
    
    target_sources(app_runner PRIVATE
        wrapper/windows.c
        wrapper/windows_keyboard.c
    )

elseif (UNIX AND NOT APPLE)
    # Linux (C source files). 
    message(STATUS "Configuring for Linux/Generic Unix (using GCC)")
    
    target_sources(app_runner PRIVATE
        wrapper/linux.c
        wrapper/linux_keyboard.c
    )

else()
    # Handle unknown platforms
    message(FATAL_ERROR "Unsupported platform detected. Cannot continue build configuration.")
endif()