/**
 * @file windows_keyboard.c
 * @brief Converts Windows virtual key codes to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#include "windows_keyboard.h"
#include <stdio.h>

/**
 * @brief Converts Windows virtual key code to IBM PC 16-bit scan code
 */
int get_scancode(WPARAM vkCode, LPARAM lParam) {
    // Get modifier states
    BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    BOOL alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    
    // Letter keys (A-Z)
    if (vkCode >= 'A' && vkCode <= 'Z') {
        // Base scan codes for letters
        const int scanCodes[26] = {
            0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
            0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
            0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
        };
        
        int idx = vkCode - 'A';
        int scanCode = scanCodes[idx];
        char lowerChar = 'a' + idx;
        char upperChar = 'A' + idx;
        
        if (alt) return (scanCode << 8) | 0x00;
        if (ctrl) return (scanCode << 8) | (idx + 1);
        if (shift) return (scanCode << 8) | upperChar;
        return (scanCode << 8) | lowerChar;
    }
    
    // Number keys (0-9)
    if (vkCode >= '0' && vkCode <= '9') {
        const int scanCodes[10] = {0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
        const char shiftChars[10] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
        
        int idx = vkCode - '0';
        int scanCode = scanCodes[idx];
        
        if (alt) return (scanCode << 8) | (0x78 + idx);
        if (ctrl && vkCode == '2') return 0x0300;
        if (ctrl && vkCode == '6') return 0x071E;
        if (shift) return (scanCode << 8) | shiftChars[idx];
        return (scanCode << 8) | vkCode;
    }
    
    // Special character keys
    switch (vkCode) {
        case VK_OEM_MINUS: // -_
            if (alt) return 0x8200;
            if (ctrl) return 0x0C1F;
            if (shift) return 0x0C5F;
            return 0x0C2D;
            
        case VK_OEM_PLUS: // =+
            if (alt) return 0x8300;
            if (shift) return 0x0D2B;
            return 0x0D3D;
            
        case VK_OEM_4: // [{
            if (alt) return 0x1A00;
            if (ctrl) return 0x1A1B;
            if (shift) return 0x1A7B;
            return 0x1A5B;
            
        case VK_OEM_6: // ]}
            if (alt) return 0x1B00;
            if (ctrl) return 0x1B1D;
            if (shift) return 0x1B7D;
            return 0x1B5D;
            
        case VK_OEM_1: // ;:
            if (alt) return 0x2700;
            if (shift) return 0x273A;
            return 0x273B;
            
        case VK_OEM_7: // '"
            if (shift) return 0x2822;
            return 0x2827;
            
        case VK_OEM_3: // `~
            if (shift) return 0x297E;
            return 0x2960;
            
        case VK_OEM_5: // \|
            if (alt) return 0x2600;
            if (ctrl) return 0x2B1C;
            if (shift) return 0x2B7C;
            return 0x2B5C;
            
        case VK_OEM_COMMA: // ,<
            if (shift) return 0x333C;
            return 0x332C;
            
        case VK_OEM_PERIOD: // .>
            if (shift) return 0x343E;
            return 0x342E;
            
        case VK_OEM_2: // /?
            if (shift) return 0x353F;
            return 0x352F;
    }
    
    // Function keys
    if (vkCode >= VK_F1 && vkCode <= VK_F12) {
        const int baseScanCodes[12] = {
            0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x85, 0x86
        };
        
        int idx = vkCode - VK_F1;
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
    switch (vkCode) {
        case VK_BACK:
            if (alt) return 0x0E00;
            if (ctrl) return 0x0E7F;
            return 0x0E08;
            
        case VK_DELETE:
            if (alt) return 0xA300;
            if (ctrl) return 0x9300;
            if (shift) return 0x532E;
            return 0x5300;
            
        case VK_DOWN:
            if (alt) return 0xA000;
            if (ctrl) return 0x9100;
            if (shift) return 0x5032;
            return 0x5000;
            
        case VK_END:
            if (alt) return 0x9F00;
            if (ctrl) return 0x7500;
            if (shift) return 0x4F31;
            return 0x4F00;
            
        case VK_RETURN:
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
            
        case VK_ESCAPE:
            if (alt) return 0x0100;
            return 0x011B;
            
        case VK_HOME:
            if (alt) return 0x9700;
            if (ctrl) return 0x7700;
            if (shift) return 0x4737;
            return 0x4700;
            
        case VK_INSERT:
            if (alt) return 0xA200;
            if (ctrl) return 0x9200;
            if (shift) return 0x5230;
            return 0x5200;
            
        case VK_LEFT:
            if (alt) return 0x9B00;
            if (ctrl) return 0x7300;
            if (shift) return 0x4B34;
            return 0x4B00;
            
        case VK_NEXT: // Page Down
            if (alt) return 0xA100;
            if (ctrl) return 0x7600;
            if (shift) return 0x5133;
            return 0x5100;
            
        case VK_PRIOR: // Page Up
            if (alt) return 0x9900;
            if (ctrl) return 0x8400;
            if (shift) return 0x4939;
            return 0x4900;
            
        case VK_RIGHT:
            if (alt) return 0x9D00;
            if (ctrl) return 0x7400;
            if (shift) return 0x4D36;
            return 0x4D00;
            
        case VK_SPACE:
            return 0x3920;
            
        case VK_TAB:
            if (alt) return 0xA500;
            if (ctrl) return 0x9400;
            if (shift) return 0x0F00;
            return 0x0F09;
            
        case VK_UP:
            if (alt) return 0x9800;
            if (ctrl) return 0x8D00;
            if (shift) return 0x4838;
            return 0x4800;
    }
    
    // Numeric keypad
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        const int scanCodes[10] = {0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49};
        const char chars[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        const int shiftedCodes[10] = {0x5230, 0x4F31, 0x5032, 0x5133, 0x4B34, 0x4C35, 0x4D36, 0x4737, 0x4838, 0x4939};
        
        int idx = vkCode - VK_NUMPAD0;
        
        if (shift) return shiftedCodes[idx];
        if (idx == 5 && ctrl) return 0x8F00;
        return (scanCodes[idx] << 8) | chars[idx];
    }
    
    switch (vkCode) {
        case VK_MULTIPLY:
            if (alt) return 0x3700;
            if (ctrl) return 0x9600;
            return 0x372A;
            
        case VK_ADD:
            if (alt) return 0x4E00;
            return 0x4E2B;
            
        case VK_DECIMAL:
            if (shift) return 0x532E;
            return 0x5300;
            
        case VK_DIVIDE:
            if (alt) return 0xA400;
            if (ctrl) return 0x9500;
            return 0x352F;
            
        case VK_SUBTRACT:
            if (alt) return 0x4A00;
            if (ctrl) return 0x8E00;
            return 0x4A2D;
    }
    
    return 0x0000; // Unmapped key
}

/**
 * @brief Converts current keyboard state to IBM PC BIOS status byte
 */
int get_statuscode(void) {
    int bios_byte = 0x00;
    
    // Right Shift (Bit 0)
    if (GetKeyState(VK_RSHIFT) & 0x8000) {
        bios_byte |= 0x01;
    }
    
    // Left Shift (Bit 1)
    if (GetKeyState(VK_LSHIFT) & 0x8000) {
        bios_byte |= 0x02;
    }
    
    // Control (Bit 2)
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        bios_byte |= 0x04;
    }
    
    // Alt (Bit 3)
    if (GetKeyState(VK_MENU) & 0x8000) {
        bios_byte |= 0x08;
    }
    
    // Scroll Lock (Bit 4)
    if (GetKeyState(VK_SCROLL) & 0x0001) {
        bios_byte |= 0x10;
    }
    
    // Num Lock (Bit 5)
    if (GetKeyState(VK_NUMLOCK) & 0x0001) {
        bios_byte |= 0x20;
    }
    
    // Caps Lock (Bit 6)
    if (GetKeyState(VK_CAPITAL) & 0x0001) {
        bios_byte |= 0x40;
    }
    
    // Insert (Bit 7)
    if (GetKeyState(VK_INSERT) & 0x0001) {
        bios_byte |= 0x80;
    }
    
    return bios_byte;
}
