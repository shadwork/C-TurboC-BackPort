set(MATRIX_SOURCES
    matrix.c
    ${WRAPPER_SOURCES}
)

# Add the executable target
# TARGET_NAME and SAMPLE_SOURCES are defined in the parent scopes
add_executable(${MATRIX_NAME} ${MATRIX_SOURCES})

# Link the libraries and frameworks to the executable
target_link_libraries(${MATRIX_NAME} PRIVATE
    ${PCCORE_NAME}
    "-framework Cocoa"
    "-framework AppKit"
)

# Set common include directories for the executable
# COMMON_INCLUDES is defined in the top-level CMakeLists.txt
target_include_directories(${MATRIX_NAME} PRIVATE ${COMMON_INCLUDES})

# --- Compilation Flags ---

# Apply general compiler flags (C, CXX, Objective-C/C++ sources)
target_compile_options(${MATRIX_NAME} PRIVATE
    -fobjc-arc
    -Wall
    -g
    -fmodules
    -fcxx-modules
)

# Define the preprocessor macro
target_compile_definitions(${MATRIX_NAME} PRIVATE
    WRAPPER
)

# --- Linking Flags ---

# Link options for the executable
target_link_options(${MATRIX_NAME} PRIVATE
    -fmodules
    -fcxx-modules
)