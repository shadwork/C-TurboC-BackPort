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

// Include cga fonts
#include "cgafont.h"

// Standard PC address for CGA video memory buffer
#define CGA_VIDEO_RAM_START 0xB8000
// Standard PC I/O port for CGA Color Select Register
// |7|6|5|4|3|2|1|0|  3D9 Color Select Register (3B9 not used)
//  | | | | | `-------- screen/border RGB
//  | | | | `--------- select intensity setting
//  | | | `---------- background intensity
//  `--------------- unused
#define CGA_COLOR_REGISTER_PORT 0x3D9
// Standard PC I/O port for CGA Mode Control Register
// |7|6|5|4|3|2|1|0|  3D8 Mode Select Register
//  | | | | | | | `---- 1 = 80x25 text, 0 = 40x25 text
//  | | | | | | `----- 1 = 320x200 graphics, 0 = text
//  | | | | | `------ 1 = B/W, 0 = color
//  | | | | `------- 1 = enable video signal
//  | | | `-------- 1 = 640x200 B/W graphics
//  | | `--------- 1 = blink, 0 = no blink
//  `------------ unused
#define CGA_MODE_CONTROL_PORT 0x3D8

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

/**
 * @brief 16-level grayscale palette for CGA B/W composite mode.
 * 
 * This palette is used when the color burst bit (bit 2) of the Color Select 
 * Register (0x3D9) is set to 1 (disabled). Each CGA color is converted to 
 * its grayscale equivalent using standard luminance conversion.
 * 
 * Formula: Gray = 0.299*R + 0.587*G + 0.114*B
 * 
 * This simulates how CGA output appears on a monochrome composite monitor
 * or when the color burst signal is disabled.
 */
static const RgbColor g_cgaGrayPalette[16] = {
    {0, 0, 0},         /* 0: Black -> Black */
    {21, 21, 21},      /* 1: Blue -> Dark Gray */
    {50, 50, 50},      /* 2: Green -> Medium-Dark Gray */
    {71, 71, 71},      /* 3: Cyan -> Medium Gray */
    {51, 51, 51},      /* 4: Red -> Medium-Dark Gray */
    {72, 72, 72},      /* 5: Magenta -> Medium Gray */
    {93, 93, 93},      /* 6: Brown -> Medium-Light Gray */
    {170, 170, 170},   /* 7: Light Gray -> Light Gray */
    {85, 85, 85},      /* 8: Dark Gray -> Dark Gray */
    {106, 106, 106},   /* 9: Bright Blue -> Medium-Light Gray */
    {135, 135, 135},   /* 10: Bright Green -> Light Gray */
    {156, 156, 156},   /* 11: Bright Cyan -> Lighter Gray */
    {136, 136, 136},   /* 12: Bright Red -> Light Gray */
    {157, 157, 157},   /* 13: Bright Magenta -> Lighter Gray */
    {242, 242, 242},   /* 14: Yellow -> Very Light Gray */
    {255, 255, 255}    /* 15: White -> White */
};

/**
 * @brief Fixed palette for 320x200 B/W mode (Mode 5) - Grayscale emulation.
 * Colors 1, 2, and 3 are set to distinct gray levels.
 */
static const RgbColor g_cgaGrayscalePalette[4] = {
    {0, 0, 0},         /* 0: Placeholder for Background/Border (Handled by 0x3D9) */
    {85, 85, 85},      /* 1: Low Gray */
    {170, 170, 170},   /* 2: Medium Gray */
    {255, 255, 255}    /* 3: White */
};

/**
 * @brief Fixed palette for 320x200 B/W mode (Mode 5) - Standard RGB output.
 * Colors 1, 2, and 3 are fixed to Cyan, Red, White.
 */
static const RgbColor g_cgaCyanRedWhitePalette[4] = {
    {0, 0, 0},         /* 0: Placeholder for Background/Border (Handled by 0x3D9) */
    {0, 170, 170},     /* 1: Cyan (from 16-color index 3) */
    {170, 0, 0},       /* 2: Red (from 16-color index 4) */
    {255, 255, 255}    /* 3: White (from 16-color index 15) */
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
 * @brief Renders the 320x200 4-color "Mode 5" (Switches between Grayscale/Cyan-Red-White).
 * Reads from pccore->memory, pccore->port 0x3D9 and 0x3D8.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render320x200x2g(IMAGE* image, PCCORE pccore);

/**
 * @brief Renders the 640x200 2-color mode.
 * Reads from pccore->memory and writes to image->raw.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render640x200x1(IMAGE* image, PCCORE pccore);

/**
 * @brief Renders the 40x25 B/W text mode (Mode 0).
 *
 * In CGA text modes, video memory is organized as character/attribute pairs.
 * Each character cell occupies 2 bytes:
 * - Byte 0: ASCII character code (index into font table)
 * - Byte 1: Attribute byte (foreground color in bits 0-3, background in bits 4-6, blink in bit 7)
 *
 * The screen layout is 40 columns × 25 rows = 1000 characters = 2000 bytes.
 * Each character is rendered using the 8x8 font, resulting in 320x200 pixels.
 *
 * The Color/B&W selection is controlled by bit 2 (0x04) of the Color Select Register (0x3D9):
 * - Bit 2 clear (color burst enabled): Use full 16-color CGA palette
 * - Bit 2 set (color burst disabled): Use grayscale palette (B/W composite mode)
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render40x25(IMAGE* image, PCCORE pccore);

#endif // CGA_H