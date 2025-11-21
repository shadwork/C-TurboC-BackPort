

# --- Variables ---

# Compiler
CC = clang

# Executable name
TARGET = sample

# Source files
# We now have two source files to compile and link
SRC = ../wrapper/macos.m ../wrapper/macos_keyboard.m ../pccore/pccore.c ../pccore/cga.c ../pccore/cgafont.c ../turboc/dos.c ../turboc/bios.c ../turboc/conio.c ../turboc/time.c ../turboc/int10.c matrix.c

# Header files (for dependency tracking)
HEADERS = ../pccore/pccore.h

# Compiler flags
CFLAGS = -fobjc-arc -Wall -g -DWRAPPER

# Linker flags
LDFLAGS = -fmodules -framework Cocoa -framework AppKit

# --- Targets ---

.PHONY: all
all: $(TARGET)

# Rule to build the target executable
# Now depends on BOTH source files and the header
$(TARGET): $(SRC) $(HEADERS)
	@echo "Compiling and linking $(TARGET)..."
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(LDFLAGS)
	@echo "Build complete."

.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM