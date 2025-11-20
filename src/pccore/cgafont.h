#ifndef CGA_FONT_H
#define CGA_FONT_H

// =================================================================================
// CGA_FONT_BOLD (8x8 Pixel Font)
//
// This array contains the bitmap data for 256 characters (ASCII 0-255)
// in an 8x8 pixel format, typically used for early text modes like CGA.
// Each character is defined by 8 bytes (rows), with the MSB (most significant bit)
// representing the leftmost pixel. A set bit (1) is the foreground pixel.
// Total size: 256 characters * 8 bytes/char = 2048 bytes.
// =================================================================================

#include <stdint.h>

extern const uint8_t CGA_FONT_BOLD[2048];

#endif // CGA_FONT_H