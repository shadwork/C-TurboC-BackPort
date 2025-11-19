/*
 * pccore.h
 *
 * Header file defining the core data structures for a simple
 * PC emulation or graphics rendering core.
 */

#include "bda.h"

// Header guard to prevent multiple inclusions
#ifndef PCCORE_H
#define PCCORE_H

// --- Constants ---

// Define buffer sizes for clarity
#define IMAGE_RAW_BUFFER_SIZE (640 * 480 * 3 * 2)
#define PCCORE_MEMORY_SIZE (1000 * 1000)
#define PCCORE_PORT_SIZE 65535

// --- Enumerations ---

/**
 * @brief Defines the available video modes.
 */
typedef enum {
    CGA320x200x2g, // 320x200, 2 bits per pixel (4 grays)
    CGA320x200x2, // 320x200, 2 bits per pixel (4 colors)
    CGA640x200x1  // 640x200, 1 bit per pixel (2 colors)
} VIDEOMODE;

// --- Structures ---

/**
 * @brief Represents a rendered image buffer.
 *
 * This structure holds the raw pixel data for a display
 * image, along with its dimensions.
 */
typedef struct {
    /**
     * @brief Raw pixel buffer.
     * The size (640*480*3*2) seems large and specific.
     * It might represent a max resolution (640x480) with
     * 3 color channels and 2 bytes per channel (or 2 frames).
     */
    unsigned char raw[IMAGE_RAW_BUFFER_SIZE];

    int width;          // Actual width of the image in pixels
    int height;         // Actual height of the image in pixels
    float aspect_ratio; // Pixel or display aspect ratio
} IMAGE;

/**
 * @brief Represents the core state of a PC.
 *
 * This structure holds the main memory, I/O port state,
 * and current operating modes.
 */
typedef struct {
    // Main system memory
    unsigned char memory[PCCORE_MEMORY_SIZE];

    // I/O port address space
    unsigned char port[PCCORE_PORT_SIZE];

    // Current video mode. See the VIDEOMODE enum.
    VIDEOMODE mode;

    // Last key pressed or keyboard state
    int key;
} PCCORE;

// --- Function Prototypes ---

/**
 * @brief Renders the PC core's memory into an image buffer.
 *
 * This function reads the state of the PC (especially its video memory,
 * based on the current 'mode') and renders the corresponding
 * graphical output into the provided IMAGE structure.
 *
 * @param image  A pointer to the IMAGE structure to be filled with
 * pixel data.
 * @param pccore The current state of the PC core to render from.
 */
void render(IMAGE* image, PCCORE pccore);

PCCORE pccore;

#endif // PCCORE_H