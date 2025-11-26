set(FIRE_SOURCES
    fire.c
    ${WRAPPER_SOURCES}
)

# Add the executable target
# TARGET_NAME and SAMPLE_SOURCES are defined in the parent scopes
add_executable(${FIRE_NAME} ${FIRE_SOURCES})

# Link the libraries and frameworks to the executable
target_link_libraries(${FIRE_NAME} PRIVATE
    ${PCCORE_NAME}
    "-framework Cocoa"
    "-framework AppKit"
)

# Set common include directories for the executable
# COMMON_INCLUDES is defined in the top-level CMakeLists.txt
target_include_directories(${FIRE_NAME} PRIVATE ${COMMON_INCLUDES})

# --- Compilation Flags ---

# Apply general compiler flags (C, CXX, Objective-C/C++ sources)
target_compile_options(${FIRE_NAME} PRIVATE
    -fobjc-arc
    -Wall
    -g
    -fmodules
    -fcxx-modules
)

# Define the preprocessor macro
target_compile_definitions(${FIRE_NAME} PRIVATE
    WRAPPER
)

# --- Linking Flags ---

# Link options for the executable
target_link_options(${FIRE_NAME} PRIVATE
    -fmodules
    -fcxx-modules
)