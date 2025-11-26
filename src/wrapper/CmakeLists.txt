# Define source files for the pccore library
# These sources are from pccore and turboc directories
set(PCCORE_SOURCES
    ../pccore/pccore.c
    ../pccore/cga.c
    ../pccore/cgafont.c
    ../turboc/dos.c
    ../turboc/bios.c
    ../turboc/int10.c
    ../turboc/conio.c
)

## 1. pccore Library
add_library(${PCCORE_NAME} STATIC ${PCCORE_SOURCES})

# Make sure the pccore library can see its own headers and the turboc headers
# COMMON_INCLUDES is a variable defined in the top-level CMakeLists.txt
target_include_directories(${PCCORE_NAME} PUBLIC ${COMMON_INCLUDES})