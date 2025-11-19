/*
 * cga.h
 *
 * Header file for specific CGA video mode rendering functions.
 * These functions are called by the main render() function.
 */

#ifndef CGA_H
#define CGA_H

// Include the core definitions, as these functions will need them
#include "pccore.h"

// Standard PC address for CGA video memory buffer
#define CGA_VIDEO_RAM_START 0xB8000
// Standard PC I/O port for CGA Color Select Register
#define CGA_COLOR_REGISTER_PORT 0x3D9

// CGA 320x200 memory layout details from cga_win.c
#define CGA_BYTES_PER_LINE 80
#define CGA_BANK_DATA_SIZE 8000
#define CGA_BANK1_OFFSET 8192

// --- Static CGA Palette ---

// Helper structure for a simple 24-bit RGB color
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

/**
 * @brief Full 16-color CGA palette lookup table.
 * (Adapted from g_cga16ColorPalette in cga_win.c)
 */
static const RgbColor g_cga16ColorPalette[16] = {
    {0, 0, 0},       /* 0: Black */
    {0, 0, 170},     /* 1: Blue */
    {0, 170, 0},     /* 2: Green */
    {0, 170, 170},   /* 3: Cyan */
    {170, 0, 0},     /* 4: Red */
    {170, 0, 170},   /* 5: Magenta */
    {170, 85, 0},    /* 6: Brown */
    {170, 170, 170}, /* 7: Light Gray */
    {85, 85, 85},    /* 8: Dark Gray */
    {85, 85, 255},   /* 9: Bright Blue */
    {85, 255, 85},   /* 10: Bright Green */
    {85, 255, 255},  /* 11: Bright Cyan */
    {255, 85, 85},   /* 12: Bright Red */
    {255, 85, 255},  /* 13: Bright Magenta */
    {255, 255, 85},  /* 14: Yellow */
    {255, 255, 255}  /* 15: White */
};

// --- Function Prototypes for CGA Modes ---

/**
 * @brief Renders the 320x200 4-color mode.
 * Reads from pccore->memory and writes to image->raw.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render320x200x2(IMAGE* image, PCCORE pccore);

/**
 * @brief Renders the 640x200 2-color mode.
 * Reads from pccore->memory and writes to image->raw.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render640x200x1(IMAGE* image, PCCORE pccore);

#endif // CGA_H