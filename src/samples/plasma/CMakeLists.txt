set(PLASMA_SOURCES
    plasma.c
    ${WRAPPER_SOURCES}
)

# Add the executable target
# TARGET_NAME and SAMPLE_SOURCES are defined in the parent scopes
add_executable(${PLASMA_NAME} ${PLASMA_SOURCES})

# Link the libraries and frameworks to the executable
target_link_libraries(${PLASMA_NAME} PRIVATE
    ${WRAPPER_LINK_LIBRARIES}
)

# Set common include directories for the executable
# COMMON_INCLUDES is defined in the top-level CMakeLists.txt
target_include_directories(${PLASMA_NAME} PRIVATE ${COMMON_INCLUDES})

# --- Compilation Flags ---

# Apply general compiler flags (C, CXX, Objective-C/C++ sources)
target_compile_options(${PLASMA_NAME} PRIVATE
    ${WRAPPER_COMPILE_OPTIONS}
)
