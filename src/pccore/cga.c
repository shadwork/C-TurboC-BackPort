#include "cga.h"

/**
 * @brief Renders the 320x200 4-color mode. (Full Implementation)
 *
 * Reads from pccore->memory (at 0xB8000) and pccore->port (at 0x3D9),
 * interprets the 2-bit pixel data, maps it to the 4-color
 * palette, and writes the final 24-bit RGB values into image->raw.
 *
 * This logic is adapted from the WM_PAINT handler in cga_win.c.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render320x200x2(IMAGE* image, PCCORE pccore) {
    int y, x;
    int is_odd, scanline_index, bank_offset, line_offset, byte_index, bit_shift;
    unsigned char pixel_byte;
    int palette_index;
    
    // Array to hold the 4 active palette indexes (0=BG, 1,2,3=FG)
    int active_palette_indexes[4];
    
    // Pointer to the start of the output RGB buffer
    unsigned char* out_pixel = image->raw;
    
    // Pointer to the start of the CGA video RAM
    // (Assuming it's at 0xB8000 in the main memory map)
    unsigned char* vram = &pccore.memory[CGA_VIDEO_RAM_START];
    
    // Get the color register value from the I/O ports
    // (Assuming it's at 0x3D9)
    unsigned char color_reg = pccore.port[CGA_COLOR_REGISTER_PORT];

    // --- Add border definitions ---
    const int border_size = 16;
    const int active_width = 320;
    const int active_height = 200;
    const int final_width = active_width + (border_size * 2);
    const int final_height = active_height + (border_size * 2);

    // 1. Set the output image dimensions
    image->width = final_width;
    image->height = final_height;
    image->aspect_ratio = 1.2f;

    // 2. Calculate Active Palette (logic from cga_win.c)
    
    // Index 0 is always the border/background color (bits 0-3)
    active_palette_indexes[0] = color_reg & 0x0F;

    // Check bit 4 (0x10) for intensity.
    int intensityOffset = (color_reg & 0x10) ? 8 : 0; /* 0 or +8 */

    // Check bit 5 (0x20) for palette select
    if ((color_reg & 0x20) == 0) /* Palette 0 */
    {
        /* Base: Green[2], Red[4], Brown[6] */
        active_palette_indexes[1] = 2 + intensityOffset;
        active_palette_indexes[2] = 4 + intensityOffset;
        active_palette_indexes[3] = 6 + intensityOffset;
    }
    else /* Palette 1 */
    {
        /* Base: Cyan[3], Magenta[5], Light Gray[7] */
        active_palette_indexes[1] = 3 + intensityOffset;
        active_palette_indexes[2] = 5 + intensityOffset;
        active_palette_indexes[3] = 7 + intensityOffset;
    }

    // 3. Get the border color
    const RgbColor* border_color = &g_cga16ColorPalette[active_palette_indexes[0]];

    // 4. Render pixels with border
    for (y = 0; y < final_height; y++)
    {
        for (x = 0; x < final_width; x++)
        {
            // Check if we are in the border area
            if (y < border_size || y >= (active_height + border_size) ||
                x < border_size || x >= (active_width + border_size))
            {
                // --- Draw Border Pixel ---
                *out_pixel++ = border_color->r;
                *out_pixel++ = border_color->g;
                *out_pixel++ = border_color->b;
            }
            else
            {
                // --- Draw Active CGA Pixel ---
                
                // Calculate coordinates relative to the active area
                int cga_y = y - border_size;
                int cga_x = x - border_size;

                // Find the correct scanline in the correct bank
                is_odd = cga_y % 2;
                scanline_index = cga_y / 2;
                bank_offset = is_odd ? CGA_BANK1_OFFSET : 0;
                line_offset = bank_offset + (scanline_index * CGA_BYTES_PER_LINE);

                // Find the byte
                byte_index = cga_x / 4;

                // Extract the 2-bit pixel from the byte
                pixel_byte = vram[line_offset + byte_index];
                bit_shift = (3 - (cga_x % 4)) * 2; /* 6, 4, 2, or 0 */
                palette_index = (pixel_byte >> bit_shift) & 0x03;

                // Get the final 16-color index from our active palette
                int final_color_index = active_palette_indexes[palette_index];

                // Get the final RGB color
                const RgbColor* final_color = &g_cga16ColorPalette[final_color_index];

                // Write the 24-bit RGB pixel to the raw image buffer
                // (Assuming image->raw is tightly packed, [R,G,B,R,G,B...])
                *out_pixel++ = final_color->r;
                *out_pixel++ = final_color->g;
                *out_pixel++ = final_color->b;
            }
        }
    }
}

/**
 * @brief Renders the 640x200 2-color mode. (Full Implementation)
 *
 * Reads from pccore->memory (at 0xB8000) and pccore->port (at 0x3D9),
 * interprets the 1-bit pixel data, maps it to the 2-color
 * palette, and writes the final 24-bit RGB values into image->raw.
 *
 * This implementation also adds a 16-pixel border.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render640x200x1(IMAGE* image, PCCORE pccore) {
    int y, x;
    int is_odd, scanline_index, bank_offset, line_offset, byte_index, bit_shift;
    unsigned char pixel_byte;
    int pixel_bit;

    const int border_size = 16;
    const int active_width = 640;
    const int active_height = 200;

    const int final_width = active_width + (border_size * 2);
    const int final_height = active_height + (border_size * 2);

    // Pointer to the start of the output RGB buffer
    unsigned char* out_pixel = image->raw;

    // Pointer to the start of the CGA video RAM
    unsigned char* vram = &pccore.memory[CGA_VIDEO_RAM_START];

    // Get the color register value from the I/O ports
    unsigned char color_reg = pccore.port[CGA_COLOR_REGISTER_PORT];

    // 1. Set the output image dimensions
    image->width = final_width;
    image->height = final_height;
    image->aspect_ratio = (float)final_width / (float)final_height;

    // 2. Determine palette for 640x200 mode
    // Bits 0-3 set the border AND the foreground color.
    // Background (pixel 0) is always black.
    int color_index = color_reg & 0x0F;
    const RgbColor* border_color = &g_cga16ColorPalette[color_index];
    const RgbColor* foreground_color = &g_cga16ColorPalette[color_index];
    const RgbColor* background_color = &g_cga16ColorPalette[0]; // Index 0 is Black

    // 3. Render pixels with border
    for (y = 0; y < final_height; y++)
    {
        for (x = 0; x < final_width; x++)
        {
            // Check if we are in the border area
            if (y < border_size || y >= (active_height + border_size) ||
                x < border_size || x >= (active_width + border_size))
            {
                // --- Draw Border Pixel ---
                *out_pixel++ = border_color->r;
                *out_pixel++ = border_color->g;
                *out_pixel++ = border_color->b;
            }
            else
            {
                // --- Draw Active CGA Pixel ---
                
                // Calculate coordinates relative to the active area
                int cga_y = y - border_size;
                int cga_x = x - border_size;

                // Find the correct scanline in the correct bank
                is_odd = cga_y % 2;
                scanline_index = cga_y / 2;
                bank_offset = is_odd ? CGA_BANK1_OFFSET : 0;
                line_offset = bank_offset + (scanline_index * CGA_BYTES_PER_LINE);

                // Find the byte
                byte_index = cga_x / 8; // 8 pixels per byte

                // Extract the 1-bit pixel from the byte
                pixel_byte = vram[line_offset + byte_index];
                bit_shift = (7 - (cga_x % 8)); // 7, 6, ..., 0
                pixel_bit = (pixel_byte >> bit_shift) & 0x01;

                // Get the final RGB color
                const RgbColor* final_color = (pixel_bit == 1) ? foreground_color : background_color;

                // Write the 24-bit RGB pixel to the raw image buffer
                *out_pixel++ = final_color->r;
                *out_pixel++ = final_color->g;
                *out_pixel++ = final_color->b;
            }
        }
    }
}

/**
 * @brief Renders the 320x200 "Mode 5" (Switches between Grayscale/Cyan-Red-White).
 *
 * This function handles the 320x200 mode when the Black & White bit (Bit 2) 
 * in the Mode Control Register (0x3D8) is set. The actual palette depends 
 * on whether we are emulating a dedicated monochrome display (Grayscale)
 * or standard RGB output (Cyan/Red/White).
 *
 * Logic:
 * If Bit 2 (B/W enable) of 0x3D8 is set:
 * - The foreground palette (indices 1, 2, 3) is fixed.
 * - We use the *Grayscale* palette for emulation (as per initial request).
 * If Bit 2 (B/W enable) of 0x3D8 is NOT set:
 * - The behavior reverts to standard Mode 4.
 *
 * For simplicity and to satisfy the prompt, we will use the **B/W bit (0x04)** * of **0x3D8** to choose between the Grayscale and Cyan-Red-White palettes.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render320x200x2g(IMAGE* image, PCCORE pccore) {
    int y, x;
    int is_odd, scanline_index, bank_offset, line_offset, byte_index, bit_shift;
    unsigned char pixel_byte;
    int palette_index;
    
    // Palette array for the 4 active colors
    RgbColor active_palette[4];
    
    // Pointer to the start of the output RGB buffer
    unsigned char* out_pixel = image->raw;
    
    // Pointer to the start of the CGA video RAM
    unsigned char* vram = &pccore.memory[CGA_VIDEO_RAM_START];
    
    // Get the color register value from Port 0x3D9 (Background/Border Color)
    unsigned char color_reg = pccore.port[CGA_COLOR_REGISTER_PORT];

    // Get the mode control register from Port 0x3D8
    unsigned char mode_reg = pccore.port[CGA_MODE_CONTROL_PORT];

    // --- Add border definitions ---
    const int border_size = 16;
    const int active_width = 320;
    const int active_height = 200;
    const int final_width = active_width + (border_size * 2);
    const int final_height = active_height + (border_size * 2);

    // 1. Set the output image dimensions
    image->width = final_width;
    image->height = final_height;
    image->aspect_ratio = 1.2f;

    // 2. Determine Palette and Background
    
    // Pointer to the fixed 3-color palette (indices 1, 2, 3)
    const RgbColor* fixed_palette;

    // Check Bit 2 (0x04) of 0x3D8. If set, this usually enables B/W mode.
    // We use this bit to select between the two requested palettes.
    if (mode_reg & 0x04) {
        // B/W bit IS set (Mode 5): Use Grayscale Emulation
        fixed_palette = g_cgaGrayscalePalette;
    } else {
        // B/W bit IS NOT set: Use the fixed Cyan-Red-White RGB palette 
        // (This simulates the fixed palette often seen when this mode is 
        // incorrectly activated or on specific hardware configurations).
        fixed_palette = g_cgaCyanRedWhitePalette;
    }

    // Color 0: Background/Border Color (from 0x3D9 bits 0-3)
    int bg_index = color_reg & 0x0F;
    RgbColor bg_color_rgb = g_cga16ColorPalette[bg_index];

    // If B/W mode is active (mode_reg & 0x04), convert background/border to grayscale
    if (mode_reg & 0x04) {
        // Simple Luma conversion for grayscale
        unsigned char bg_luma = (unsigned char)((0.299f * bg_color_rgb.r) + 
                                                (0.587f * bg_color_rgb.g) + 
                                                (0.114f * bg_color_rgb.b));
        active_palette[0].r = bg_luma;
        active_palette[0].g = bg_luma;
        active_palette[0].b = bg_luma;
    } else {
        // If not in B/W mode, use the full RGB background color
        active_palette[0] = bg_color_rgb;
    }

    // Set the foreground colors (indices 1, 2, 3) from the selected fixed palette
    active_palette[1] = fixed_palette[1];
    active_palette[2] = fixed_palette[2];
    active_palette[3] = fixed_palette[3];


    // 3. Set the border color (which is index 0)
    const RgbColor* border_color = &active_palette[0];

    // 4. Render pixels
    for (y = 0; y < final_height; y++)
    {
        for (x = 0; x < final_width; x++)
        {
            // Check if we are in the border area
            if (y < border_size || y >= (active_height + border_size) ||
                x < border_size || x >= (active_width + border_size))
            {
                // --- Draw Border Pixel ---
                *out_pixel++ = border_color->r;
                *out_pixel++ = border_color->g;
                *out_pixel++ = border_color->b;
            }
            else
            {
                // --- Draw Active CGA Pixel ---
                
                // Calculate coordinates relative to the active area
                int cga_y = y - border_size;
                int cga_x = x - border_size;

                // Find the correct scanline in the correct bank (320x200 interleaved)
                is_odd = cga_y % 2;
                scanline_index = cga_y / 2;
                bank_offset = is_odd ? CGA_BANK1_OFFSET : 0;
                line_offset = bank_offset + (scanline_index * CGA_BYTES_PER_LINE);

                // Find the byte
                byte_index = cga_x / 4;

                // Extract the 2-bit pixel from the byte
                pixel_byte = vram[line_offset + byte_index];
                bit_shift = (3 - (cga_x % 4)) * 2; /* 6, 4, 2, or 0 */
                palette_index = (pixel_byte >> bit_shift) & 0x03;

                // Get the final RGB color from our local active palette
                const RgbColor* final_color = &active_palette[palette_index];

                *out_pixel++ = final_color->r;
                *out_pixel++ = final_color->g;
                *out_pixel++ = final_color->b;
            }
        }
    }
}

/**
 * @brief Renders the 40x25 B/W text mode (Mode 0) with support for blinking.
 *
 * Blinking is enabled globally by Bit 5 of the Mode Select Register (3D8).
 * The current blink phase is provided by the pccore.blink (0 or 1).
 * If blinking is enabled and active (pccore.blink == 1), any character with
 * attribute Bit 7 set will have its foreground color replaced by its background color.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render40x25(IMAGE* image, PCCORE pccore) {
    // --- Constants and Setup ---
    const int COLS = 40;
    const int ROWS = 25;
    const int CHAR_WIDTH = 8;
    const int CHAR_HEIGHT = 8;
    const int border_size = 16;
    
    const int active_width = COLS * CHAR_WIDTH;   // 320 pixels
    const int active_height = ROWS * CHAR_HEIGHT; // 200 pixels
    const int final_width = active_width + (border_size * 2);
    const int final_height = active_height + (border_size * 2);
    
    // Pointer to video RAM (starts at 0xB8000)
    unsigned char* vram = &pccore.memory[CGA_VIDEO_RAM_START];
    
    // Pointer to output buffer
    unsigned char* out_pixel = image->raw;
    
    // Get the Color Select Register (3D9)
    unsigned char color_reg = pccore.port[CGA_COLOR_REGISTER_PORT];
    
    // Get the Mode Select Register (3D8)
    unsigned char mode_reg = pccore.port[CGA_MODE_CONTROL_PORT];

    // Check bit 2 (0x04) of 3D9 - Color Burst Enable/Disable
    // 0 = Color burst enabled (use color palette), 1 = Color burst disabled (grayscale)
    int use_grayscale = (mode_reg & 0x04) ? 1 : 0;
    
    // Select the appropriate palette
    const RgbColor* palette = use_grayscale ? g_cgaGrayPalette : g_cga16ColorPalette;
    
    // Get border color from 3D9 register (bits 0-3)
    int border_color_index = color_reg & 0x0F;
    const RgbColor* border_color = &palette[border_color_index];
    
    // --- Blinking Logic Initialization ---
    
    // Check 3D8 Bit 5 (0x20) - 1 = blinking on, 0 = 16-color background
    int global_blink_enabled = (mode_reg & 0x20) != 0;
    
    // Determine if the blink effect should be applied for this frame
    // This is true if global blink is enabled AND the core's blink phase is 1.
    int blink_active_this_frame = global_blink_enabled && (pccore.blink == 1);
    
    // Set image dimensions
    image->width = final_width;
    image->height = final_height;
    image->aspect_ratio = 1.2f; // CGA aspect ratio
    
    // --- Rendering Loop ---
    for (int y = 0; y < final_height; y++) {
        for (int x = 0; x < final_width; x++) {
            // Check if we're in the border area
            if (y < border_size || y >= (active_height + border_size) ||
                x < border_size || x >= (active_width + border_size)) {
                // Draw border pixel
                *out_pixel++ = border_color->r;
                *out_pixel++ = border_color->g;
                *out_pixel++ = border_color->b;
            } else {
                // Calculate position within active area
                int cga_y = y - border_size;
                int cga_x = x - border_size;
                
                // Calculate which character cell we're in
                int char_col = cga_x / CHAR_WIDTH;
                int char_row = cga_y / CHAR_HEIGHT;
                
                // Calculate position within the character (0-7 for both x and y)
                int pixel_x_in_char = cga_x % CHAR_WIDTH;
                int pixel_y_in_char = cga_y % CHAR_HEIGHT;
                
                // Calculate offset in video RAM (2 bytes per character)
                int vram_offset = (char_row * COLS + char_col) * 2;
                
                // Get character code and attribute
                unsigned char char_code = vram[vram_offset];
                unsigned char attribute = vram[vram_offset + 1];
                
                // --- Color and Blink Extraction ---
                int fg_color_index = attribute & 0x0F; // Bits 0-3: Foreground
                int bg_color_index;
                int char_blink_flag = 0;

                if (global_blink_enabled) {
                    // Blink enabled (3D8 Bit 5 is 1): Bit 7 of attribute is the blink flag.
                    bg_color_index = (attribute >> 4) & 0x07; // Background is 8 colors (Bits 4-6)
                    char_blink_flag = attribute & 0x80;
                } else {
                    // Blink disabled (3D8 Bit 5 is 0): Bit 7 of attribute is the high BG color bit.
                    bg_color_index = (attribute >> 4) & 0x0F; // Background is 16 colors (Bits 4-7)
                    // char_blink_flag remains 0 (no individual blinking possible)
                }

                // --- Apply Blink Effect ---
                if (blink_active_this_frame && char_blink_flag) {
                    // If global blink is active this frame AND the character's blink flag is set,
                    // the foreground color is replaced by the background color.
                    fg_color_index = bg_color_index;
                }
                
                // Get the font bitmap for this character and row
                unsigned char font_byte = CGA_FONT_BOLD[char_code * 8 + pixel_y_in_char];
                
                // Check if this pixel is set in the font (MSB is leftmost pixel)
                int bit_shift = 7 - pixel_x_in_char;
                int pixel_set = (font_byte >> bit_shift) & 0x01;
                
                // Select foreground or background color
                const RgbColor* pixel_color = pixel_set ? 
                    &palette[fg_color_index] : 
                    &palette[bg_color_index];
                
                // Write the pixel to the output buffer
                *out_pixel++ = pixel_color->r;
                *out_pixel++ = pixel_color->g;
                *out_pixel++ = pixel_color->b;
            }
        }
    }
}

/**
 * @brief Renders the 80x25 text mode (Mode 1) with support for blinking and B/W selection.
 *
 * In CGA text modes, video memory is organized as character/attribute pairs:
 * - Byte 0: ASCII character code (index into font table)
 * - Byte 1: Attribute byte (Foreground/Background/Blink)
 *
 * The screen layout is 80 columns × 25 rows = 2000 characters = 4000 bytes.
 * Each character is rendered using the 8x8 font, resulting in 640x200 pixels.
 *
 * Palette selection (Color vs. Grayscale) is controlled by:
 * - Mode Control Register (0x3D8) Bit 2 (0x04): 1 = B/W, 0 = Color.
 *
 * @param image  Pointer to the output image buffer.
 * @param pccore A const pointer to the PC core state.
 */
void render80x25(IMAGE* image, PCCORE pccore) {
    // --- Constants and Setup ---
    const int COLS = 80;
    const int ROWS = 25;
    const int CHAR_WIDTH = 8;
    const int CHAR_HEIGHT = 8;
    const int border_size = 16;
    
    const int active_width = COLS * CHAR_WIDTH;   // 640 pixels
    const int active_height = ROWS * CHAR_HEIGHT; // 200 pixels
    const int final_width = active_width + (border_size * 2);
    const int final_height = active_height + (border_size * 2);
    
    // Pointer to video RAM (starts at 0xB8000)
    unsigned char* vram = &pccore.memory[CGA_VIDEO_RAM_START];
    
    // Pointer to output buffer
    unsigned char* out_pixel = image->raw;
    
    // Get the Color Select Register (0x3D9)
    unsigned char color_reg = pccore.port[CGA_COLOR_REGISTER_PORT];
    
    // Get the Mode Select Register (0x3D8)
    unsigned char mode_reg = pccore.port[CGA_MODE_CONTROL_PORT];

    // --- Palette Selection (Controlled by 0x3D8 Bit 2) ---
    // Check Bit 2 (0x04) of 0x3D8: 1 = B/W (Grayscale), 0 = Color
    int use_grayscale = (mode_reg & 0x04) ? 1 : 0;
    
    // Select the appropriate palette
    // Note: The original code in cga.h suggests 3D9 controls color burst. 
    // This implementation follows the 3D8 Bit 2 control as commonly documented 
    // for B/W mode selection (Mode 0, 1). We use the g_cgaGrayPalette for grayscale.
    const RgbColor* palette = use_grayscale ? g_cgaGrayPalette : g_cga16ColorPalette;
    
    // Get border color from 3D9 register (bits 0-3)
    int border_color_index = color_reg & 0x0F;
    const RgbColor* border_color = &palette[border_color_index];
    
    // --- Blinking Logic Initialization (Controlled by 0x3D8 Bit 5) ---
    
    // Check 3D8 Bit 5 (0x20) - 1 = blinking on, 0 = 16-color background
    int global_blink_enabled = (mode_reg & 0x20) != 0;
    
    // Determine if the blink effect should be applied for this frame
    int blink_active_this_frame = global_blink_enabled && (pccore.blink == 1);
    
    // Set image dimensions
    image->width = final_width;
    image->height = final_height;
    image->aspect_ratio = (float)final_width / (float)final_height; // 672/232 = ~2.9, but active area is 640/200 = 3.2
    
    // --- Rendering Loop ---
    for (int y = 0; y < final_height; y++) {
        for (int x = 0; x < final_width; x++) {
            // Check if we're in the border area
            if (y < border_size || y >= (active_height + border_size) ||
                x < border_size || x >= (active_width + border_size)) {
                // Draw border pixel
                *out_pixel++ = border_color->r;
                *out_pixel++ = border_color->g;
                *out_pixel++ = border_color->b;
            } else {
                // Calculate position within active area
                int cga_y = y - border_size;
                int cga_x = x - border_size;
                
                // Calculate which character cell we're in
                int char_col = cga_x / CHAR_WIDTH;
                int char_row = cga_y / CHAR_HEIGHT;
                
                // Calculate position within the character (0-7 for both x and y)
                int pixel_x_in_char = cga_x % CHAR_WIDTH;
                int pixel_y_in_char = cga_y % CHAR_HEIGHT;
                
                // Calculate offset in video RAM (2 bytes per character)
                // Offset = (row * COLS + col) * 2
                int vram_offset = (char_row * COLS + char_col) * 2;
                
                // Get character code and attribute
                unsigned char char_code = vram[vram_offset];
                unsigned char attribute = vram[vram_offset + 1];
                
                // --- Color and Blink Extraction ---
                int fg_color_index = attribute & 0x0F; // Bits 0-3: Foreground (16 colors)
                int bg_color_index;
                int char_blink_flag = 0;

                if (global_blink_enabled) {
                    // Blink enabled (3D8 Bit 5 is 1): Bit 7 of attribute is the blink flag.
                    bg_color_index = (attribute >> 4) & 0x07; // Background is 8 colors (Bits 4-6)
                    char_blink_flag = attribute & 0x80;
                } else {
                    // Blink disabled (3D8 Bit 5 is 0): Bit 7 of attribute is the high BG color bit.
                    bg_color_index = (attribute >> 4) & 0x0F; // Background is 16 colors (Bits 4-7)
                    // char_blink_flag remains 0
                }

                // --- Apply Blink Effect ---
                if (blink_active_this_frame && char_blink_flag) {
                    // If global blink is active this frame AND the character's blink flag is set,
                    // the foreground color is replaced by the background color.
                    fg_color_index = bg_color_index;
                }
                
                // Get the font bitmap for this character and row
                // Each character has 8 bytes of font data
                unsigned char font_byte = CGA_FONT_BOLD[char_code * CHAR_HEIGHT + pixel_y_in_char];
                
                // Check if this pixel is set in the font (MSB is leftmost pixel, 7 - x)
                int bit_shift = 7 - pixel_x_in_char;
                int pixel_set = (font_byte >> bit_shift) & 0x01;
                
                // Select foreground or background color
                const RgbColor* pixel_color = pixel_set ? 
                    &palette[fg_color_index] : 
                    &palette[bg_color_index];
                
                // Write the pixel to the output buffer
                *out_pixel++ = pixel_color->r;
                *out_pixel++ = pixel_color->g;
                *out_pixel++ = pixel_color->b;
            }
        }
    }
}