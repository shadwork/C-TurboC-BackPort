/**
 * @file linux_keyboard.c
 * @brief Converts Linux GDK key codes to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#include "linux_keyboard.h"
#include <gdk/gdkkeysyms.h>
#include <stdio.h>

/**
 * @brief Converts GDK key event to IBM PC 16-bit scan code
 */
int get_scancode(GdkEventKey *event) {
    guint keyval = event->keyval;
    GdkModifierType state = event->state;
    
    // Extract modifier states
    gboolean shift = (state & GDK_SHIFT_MASK) != 0;
    gboolean ctrl = (state & GDK_CONTROL_MASK) != 0;
    gboolean alt = (state & GDK_MOD1_MASK) != 0;
    
    // Letter keys (a-z, A-Z)
    if ((keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) || 
        (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z)) {
        
        // Normalize to lowercase for indexing
        int idx;
        if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) {
            idx = keyval - GDK_KEY_a;
        } else {
            idx = keyval - GDK_KEY_A;
        }
        
        const int scanCodes[26] = {
            0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
            0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
            0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
        };
        
        int scanCode = scanCodes[idx];
        char lowerChar = 'a' + idx;
        char upperChar = 'A' + idx;
        
        if (alt) return (scanCode << 8) | 0x00;
        if (ctrl) return (scanCode << 8) | (idx + 1);
        if (shift) return (scanCode << 8) | upperChar;
        return (scanCode << 8) | lowerChar;
    }
    
    // Number keys (0-9)
    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
        const int scanCodes[10] = {0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
        const char shiftChars[10] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
        
        int idx = keyval - GDK_KEY_0;
        int scanCode = scanCodes[idx];
        
        if (alt) return (scanCode << 8) | (0x78 + idx);
        if (ctrl && keyval == GDK_KEY_2) return 0x0300;
        if (ctrl && keyval == GDK_KEY_6) return 0x071E;
        if (shift) return (scanCode << 8) | shiftChars[idx];
        return (scanCode << 8) | keyval;
    }
    
    // Special character keys
    switch (keyval) {
        case GDK_KEY_minus:
        case GDK_KEY_underscore:
            if (alt) return 0x8200;
            if (ctrl) return 0x0C1F;
            if (shift) return 0x0C5F;
            return 0x0C2D;
            
        case GDK_KEY_equal:
        case GDK_KEY_plus:
            if (alt) return 0x8300;
            if (shift) return 0x0D2B;
            return 0x0D3D;
            
        case GDK_KEY_bracketleft:
        case GDK_KEY_braceleft:
            if (alt) return 0x1A00;
            if (ctrl) return 0x1A1B;
            if (shift) return 0x1A7B;
            return 0x1A5B;
            
        case GDK_KEY_bracketright:
        case GDK_KEY_braceright:
            if (alt) return 0x1B00;
            if (ctrl) return 0x1B1D;
            if (shift) return 0x1B7D;
            return 0x1B5D;
            
        case GDK_KEY_semicolon:
        case GDK_KEY_colon:
            if (alt) return 0x2700;
            if (shift) return 0x273A;
            return 0x273B;
            
        case GDK_KEY_apostrophe:
        case GDK_KEY_quotedbl:
            if (shift) return 0x2822;
            return 0x2827;
            
        case GDK_KEY_grave:
        case GDK_KEY_asciitilde:
            if (shift) return 0x297E;
            return 0x2960;
            
        case GDK_KEY_backslash:
        case GDK_KEY_bar:
            if (alt) return 0x2600;
            if (ctrl) return 0x2B1C;
            if (shift) return 0x2B7C;
            return 0x2B5C;
            
        case GDK_KEY_comma:
        case GDK_KEY_less:
            if (shift) return 0x333C;
            return 0x332C;
            
        case GDK_KEY_period:
        case GDK_KEY_greater:
            if (shift) return 0x343E;
            return 0x342E;
            
        case GDK_KEY_slash:
        case GDK_KEY_question:
            if (shift) return 0x353F;
            return 0x352F;
    }
    
    // Function keys
    if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F12) {
        const int baseScanCodes[12] = {
            0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x85, 0x86
        };
        
        int idx = keyval - GDK_KEY_F1;
        int scanCode = baseScanCodes[idx];
        
        if (alt) {
            const int altCodes[12] = {0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x8B, 0x8C};
            return (altCodes[idx] << 8);
        }
        if (ctrl) {
            const int ctrlCodes[12] = {0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x89, 0x8A};
            return (ctrlCodes[idx] << 8);
        }
        if (shift) {
            const int shiftCodes[12] = {0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x87, 0x88};
            return (shiftCodes[idx] << 8);
        }
        return (scanCode << 8);
    }
    
    // Special keys
    switch (keyval) {
        case GDK_KEY_BackSpace:
            if (alt) return 0x0E00;
            if (ctrl) return 0x0E7F;
            return 0x0E08;
            
        case GDK_KEY_Delete:
            if (alt) return 0xA300;
            if (ctrl) return 0x9300;
            if (shift) return 0x532E;
            return 0x5300;
            
        case GDK_KEY_Down:
            if (alt) return 0xA000;
            if (ctrl) return 0x9100;
            if (shift) return 0x5032;
            return 0x5000;
            
        case GDK_KEY_End:
            if (alt) return 0x9F00;
            if (ctrl) return 0x7500;
            if (shift) return 0x4F31;
            return 0x4F00;
            
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
            
        case GDK_KEY_Escape:
            if (alt) return 0x0100;
            return 0x011B;
            
        case GDK_KEY_Home:
            if (alt) return 0x9700;
            if (ctrl) return 0x7700;
            if (shift) return 0x4737;
            return 0x4700;
            
        case GDK_KEY_Insert:
            if (alt) return 0xA200;
            if (ctrl) return 0x9200;
            if (shift) return 0x5230;
            return 0x5200;
            
        case GDK_KEY_Left:
            if (alt) return 0x9B00;
            if (ctrl) return 0x7300;
            if (shift) return 0x4B34;
            return 0x4B00;
            
        case GDK_KEY_Page_Down:
            if (alt) return 0xA100;
            if (ctrl) return 0x7600;
            if (shift) return 0x5133;
            return 0x5100;
            
        case GDK_KEY_Page_Up:
            if (alt) return 0x9900;
            if (ctrl) return 0x8400;
            if (shift) return 0x4939;
            return 0x4900;
            
        case GDK_KEY_Right:
            if (alt) return 0x9D00;
            if (ctrl) return 0x7400;
            if (shift) return 0x4D36;
            return 0x4D00;
            
        case GDK_KEY_space:
            return 0x3920;
            
        case GDK_KEY_Tab:
        case GDK_KEY_ISO_Left_Tab:
            if (alt) return 0xA500;
            if (ctrl) return 0x9400;
            if (shift) return 0x0F00;
            return 0x0F09;
            
        case GDK_KEY_Up:
            if (alt) return 0x9800;
            if (ctrl) return 0x8D00;
            if (shift) return 0x4838;
            return 0x4800;
    }
    
    // Numeric keypad
    if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9) {
        const int scanCodes[10] = {0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49};
        const char chars[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        const int shiftedCodes[10] = {0x5230, 0x4F31, 0x5032, 0x5133, 0x4B34, 0x4C35, 0x4D36, 0x4737, 0x4838, 0x4939};
        
        int idx = keyval - GDK_KEY_KP_0;
        
        if (shift) return shiftedCodes[idx];
        if (idx == 5 && ctrl) return 0x8F00;
        return (scanCodes[idx] << 8) | chars[idx];
    }
    
    switch (keyval) {
        case GDK_KEY_KP_Multiply:
            if (alt) return 0x3700;
            if (ctrl) return 0x9600;
            return 0x372A;
            
        case GDK_KEY_KP_Add:
            if (alt) return 0x4E00;
            return 0x4E2B;
            
        case GDK_KEY_KP_Decimal:
        case GDK_KEY_KP_Delete:
            if (shift) return 0x532E;
            return 0x5300;
            
        case GDK_KEY_KP_Divide:
            if (alt) return 0xA400;
            if (ctrl) return 0x9500;
            return 0x352F;
            
        case GDK_KEY_KP_Subtract:
            if (alt) return 0x4A00;
            if (ctrl) return 0x8E00;
            return 0x4A2D;
    }
    
    return 0x0000; // Unmapped key
}

/**
 * @brief Converts GDK key event to IBM PC BIOS status byte
 */
int get_statuscode(GdkEventKey *event) {
    GdkModifierType state = event->state;
    int bios_byte = 0x00;
    
    // Check shift keys
    // GDK doesn't easily distinguish left/right shift in modifier flags
    // We'll treat any shift as left shift for simplicity
    if (state & GDK_SHIFT_MASK) {
        bios_byte |= 0x02; // Bit 1: Left Shift
        // Note: Right shift detection would require hardware scan codes
    }
    
    // Control (Bit 2)
    if (state & GDK_CONTROL_MASK) {
        bios_byte |= 0x04;
    }
    
    // Alt (Bit 3)
    if (state & GDK_MOD1_MASK) {
        bios_byte |= 0x08;
    }
    
    // Scroll Lock (Bit 4)
    if (state & GDK_SCROLL_LOCK_MASK) {
        bios_byte |= 0x10;
    }
    
    // Num Lock (Bit 5)
    if (state & GDK_MOD2_MASK) {
        bios_byte |= 0x20;
    }
    
    // Caps Lock (Bit 6)
    if (state & GDK_LOCK_MASK) {
        bios_byte |= 0x40;
    }
    
    // Insert (Bit 7)
    // GDK doesn't have a standard insert toggle state modifier
    // Would need to track this separately
    
    return bios_byte;
}
