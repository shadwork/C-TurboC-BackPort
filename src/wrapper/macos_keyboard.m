/**
 * @file macos_key.m
 * @brief Converts macOS NSEvent key codes to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

@import AppKit;

/**
 * @brief Converts macOS NSEvent to IBM PC 16-bit scan code
 * @param event The NSEvent containing key code and modifier flags
 * @return 16-bit IBM PC scan code (high byte = scan code, low byte = ASCII)
 *         Returns 0x0000 if the key is not mapped
 */
int get_scancode(NSEvent *event) {
    unsigned short keyCode = [event keyCode];
    NSEventModifierFlags flags = [event modifierFlags];
    
    // Extract modifier states
    BOOL shift = (flags & NSEventModifierFlagShift) != 0;
    BOOL ctrl = (flags & NSEventModifierFlagControl) != 0;
    BOOL alt = (flags & NSEventModifierFlagOption) != 0;
    
    // Letter keys (A-Z)
    // macOS key codes: 0=A, 11=B, 8=C, 2=D, 14=E, 3=F, 5=G, 4=H, 34=I, 38=J,
    //                  40=K, 37=L, 46=M, 45=N, 31=O, 35=P, 12=Q, 15=R, 1=S,
    //                  17=T, 32=U, 9=V, 13=W, 7=X, 16=Y, 6=Z
    switch (keyCode) {
        // Letters
        case 0:  // A
            if (alt) return 0x1E00;
            if (ctrl) return 0x1E01;
            if (shift) return 0x1E41;
            return 0x1E61;
        case 11: // B
            if (alt) return 0x3000;
            if (ctrl) return 0x3002;
            if (shift) return 0x3042;
            return 0x3062;
        case 8:  // C
            if (alt) return 0x2E00;
            if (ctrl) return 0x2E03;
            if (shift) return 0x2E43;
            return 0x2E63;
        case 2:  // D
            if (alt) return 0x2000;
            if (ctrl) return 0x2004;
            if (shift) return 0x2044;
            return 0x2064;
        case 14: // E
            if (alt) return 0x1200;
            if (ctrl) return 0x1205;
            if (shift) return 0x1245;
            return 0x1265;
        case 3:  // F
            if (alt) return 0x2100;
            if (ctrl) return 0x2106;
            if (shift) return 0x2146;
            return 0x2166;
        case 5:  // G
            if (alt) return 0x2200;
            if (ctrl) return 0x2207;
            if (shift) return 0x2247;
            return 0x2267;
        case 4:  // H
            if (alt) return 0x2300;
            if (ctrl) return 0x2308;
            if (shift) return 0x2348;
            return 0x2368;
        case 34: // I
            if (alt) return 0x1700;
            if (ctrl) return 0x1709;
            if (shift) return 0x1749;
            return 0x1769;
        case 38: // J
            if (alt) return 0x2400;
            if (ctrl) return 0x240A;
            if (shift) return 0x244A;
            return 0x246A;
        case 40: // K
            if (alt) return 0x2500;
            if (ctrl) return 0x250B;
            if (shift) return 0x254B;
            return 0x256B;
        case 37: // L
            if (alt) return 0x2600;
            if (ctrl) return 0x260C;
            if (shift) return 0x264C;
            return 0x266C;
        case 46: // M
            if (alt) return 0x3200;
            if (ctrl) return 0x320D;
            if (shift) return 0x324D;
            return 0x326D;
        case 45: // N
            if (alt) return 0x3100;
            if (ctrl) return 0x310E;
            if (shift) return 0x314E;
            return 0x316E;
        case 31: // O
            if (alt) return 0x1800;
            if (ctrl) return 0x180F;
            if (shift) return 0x184F;
            return 0x186F;
        case 35: // P
            if (alt) return 0x1900;
            if (ctrl) return 0x1910;
            if (shift) return 0x1950;
            return 0x1970;
        case 12: // Q
            if (alt) return 0x1000;
            if (ctrl) return 0x1011;
            if (shift) return 0x1051;
            return 0x1071;
        case 15: // R
            if (alt) return 0x1300;
            if (ctrl) return 0x1312;
            if (shift) return 0x1352;
            return 0x1372;
        case 1:  // S
            if (alt) return 0x1F00;
            if (ctrl) return 0x1F13;
            if (shift) return 0x1F53;
            return 0x1F73;
        case 17: // T
            if (alt) return 0x1400;
            if (ctrl) return 0x1414;
            if (shift) return 0x1454;
            return 0x1474;
        case 32: // U
            if (alt) return 0x1600;
            if (ctrl) return 0x1615;
            if (shift) return 0x1655;
            return 0x1675;
        case 9:  // V
            if (alt) return 0x2F00;
            if (ctrl) return 0x2F16;
            if (shift) return 0x2F56;
            return 0x2F76;
        case 13: // W
            if (alt) return 0x1100;
            if (ctrl) return 0x1117;
            if (shift) return 0x1157;
            return 0x1177;
        case 7:  // X
            if (alt) return 0x2D00;
            if (ctrl) return 0x2D18;
            if (shift) return 0x2D58;
            return 0x2D78;
        case 16: // Y
            if (alt) return 0x1500;
            if (ctrl) return 0x1519;
            if (shift) return 0x1559;
            return 0x1579;
        case 6:  // Z
            if (alt) return 0x2C00;
            if (ctrl) return 0x2C1A;
            if (shift) return 0x2C5A;
            return 0x2C7A;
            
        // Number keys (1-0)
        case 18: // 1
            if (alt) return 0x7800;
            if (shift) return 0x0221;
            return 0x0231;
        case 19: // 2
            if (alt) return 0x7900;
            if (ctrl) return 0x0300;
            if (shift) return 0x0340; // @
            return 0x0332;
        case 20: // 3
            if (alt) return 0x7A00;
            if (shift) return 0x0423; // #
            return 0x0433;
        case 21: // 4
            if (alt) return 0x7B00;
            if (shift) return 0x0524; // $
            return 0x0534;
        case 23: // 5
            if (alt) return 0x7C00;
            if (shift) return 0x0625; // %
            return 0x0635;
        case 22: // 6
            if (alt) return 0x7D00;
            if (ctrl) return 0x071E;
            if (shift) return 0x075E; // ^
            return 0x0736;
        case 26: // 7
            if (alt) return 0x7E00;
            if (shift) return 0x0826; // &
            return 0x0837;
        case 28: // 8
            if (alt) return 0x7F00;
            if (shift) return 0x092A; // *
            return 0x0938;
        case 25: // 9
            if (alt) return 0x8000;
            if (shift) return 0x0A28; // (
            return 0x0A39;
        case 29: // 0
            if (alt) return 0x8100;
            if (shift) return 0x0B29; // )
            return 0x0B30;
            
        // Symbol keys
        case 27: // - (minus/underscore)
            if (alt) return 0x8200;
            if (ctrl) return 0x0C1F;
            if (shift) return 0x0C5F; // _
            return 0x0C2D;
        case 24: // = (equals/plus)
            if (alt) return 0x8300;
            if (shift) return 0x0D2B; // +
            return 0x0D3D;
        case 33: // [ (left bracket)
            if (alt) return 0x1A00;
            if (ctrl) return 0x1A1B;
            if (shift) return 0x1A7B; // {
            return 0x1A5B;
        case 30: // ] (right bracket)
            if (alt) return 0x1B00;
            if (ctrl) return 0x1B1D;
            if (shift) return 0x1B7D; // }
            return 0x1B5D;
        case 41: // ; (semicolon)
            if (alt) return 0x2700;
            if (shift) return 0x273A; // :
            return 0x273B;
        case 39: // ' (apostrophe)
            if (shift) return 0x2822; // "
            return 0x2827;
        case 50: // ` (grave accent/tilde)
            if (shift) return 0x297E; // ~
            return 0x2960;
        case 42: // \ (backslash)
            if (alt) return 0x2600;
            if (ctrl) return 0x2B1C;
            if (shift) return 0x2B7C; // |
            return 0x2B5C;
        case 43: // , (comma)
            if (shift) return 0x333C; // <
            return 0x332C;
        case 47: // . (period)
            if (shift) return 0x343E; // >
            return 0x342E;
        case 44: // / (slash)
            if (shift) return 0x353F; // ?
            return 0x352F;
            
        // Function keys
        case 122: // F1
            if (alt) return 0x6800;
            if (ctrl) return 0x5E00;
            if (shift) return 0x5400;
            return 0x3B00;
        case 120: // F2
            if (alt) return 0x6900;
            if (ctrl) return 0x5F00;
            if (shift) return 0x5500;
            return 0x3C00;
        case 99: // F3
            if (alt) return 0x6A00;
            if (ctrl) return 0x6000;
            if (shift) return 0x5600;
            return 0x3D00;
        case 118: // F4
            if (alt) return 0x6B00;
            if (ctrl) return 0x6100;
            if (shift) return 0x5700;
            return 0x3E00;
        case 96: // F5
            if (alt) return 0x6C00;
            if (ctrl) return 0x6200;
            if (shift) return 0x5800;
            return 0x3F00;
        case 97: // F6
            if (alt) return 0x6D00;
            if (ctrl) return 0x6300;
            if (shift) return 0x5900;
            return 0x4000;
        case 98: // F7
            if (alt) return 0x6E00;
            if (ctrl) return 0x6400;
            if (shift) return 0x5A00;
            return 0x4100;
        case 100: // F8
            if (alt) return 0x6F00;
            if (ctrl) return 0x6500;
            if (shift) return 0x5B00;
            return 0x4200;
        case 101: // F9
            if (alt) return 0x7000;
            if (ctrl) return 0x6600;
            if (shift) return 0x5C00;
            return 0x4300;
        case 109: // F10
            if (alt) return 0x7100;
            if (ctrl) return 0x6700;
            if (shift) return 0x5D00;
            return 0x4400;
        case 103: // F11
            if (alt) return 0x8B00;
            if (ctrl) return 0x8900;
            if (shift) return 0x8700;
            return 0x8500;
        case 111: // F12
            if (alt) return 0x8C00;
            if (ctrl) return 0x8A00;
            if (shift) return 0x8800;
            return 0x8600;
            
        // Special keys
        case 51: // Backspace
            if (alt) return 0x0E00;
            if (ctrl) return 0x0E7F;
            return 0x0E08;
        case 117: // Delete
            if (alt) return 0xA300;
            if (ctrl) return 0x9300;
            if (shift) return 0x532E;
            return 0x5300;
        case 125: // Down Arrow
            if (alt) return 0xA000;
            if (ctrl) return 0x9100;
            if (shift) return 0x5032;
            return 0x5000;
        case 119: // End
            if (alt) return 0x9F00;
            if (ctrl) return 0x7500;
            if (shift) return 0x4F31;
            return 0x4F00;
        case 36: // Enter/Return
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
        case 53: // Escape
            if (alt) return 0x0100;
            return 0x011B;
        case 115: // Home
            if (alt) return 0x9700;
            if (ctrl) return 0x7700;
            if (shift) return 0x4737;
            return 0x4700;
        case 114: // Insert (Help on Mac)
            if (alt) return 0xA200;
            if (ctrl) return 0x9200;
            if (shift) return 0x5230;
            return 0x5200;
        case 123: // Left Arrow
            if (alt) return 0x9B00;
            if (ctrl) return 0x7300;
            if (shift) return 0x4B34;
            return 0x4B00;
        case 121: // Page Down
            if (alt) return 0xA100;
            if (ctrl) return 0x7600;
            if (shift) return 0x5133;
            return 0x5100;
        case 116: // Page Up
            if (alt) return 0x9900;
            if (ctrl) return 0x8400;
            if (shift) return 0x4939;
            return 0x4900;
        case 124: // Right Arrow
            if (alt) return 0x9D00;
            if (ctrl) return 0x7400;
            if (shift) return 0x4D36;
            return 0x4D00;
        case 49: // Space
            return 0x3920;
        case 48: // Tab
            if (alt) return 0xA500;
            if (ctrl) return 0x9400;
            if (shift) return 0x0F00;
            return 0x0F09;
        case 126: // Up Arrow
            if (alt) return 0x9800;
            if (ctrl) return 0x8D00;
            if (shift) return 0x4838;
            return 0x4800;
            
        // Keypad
        case 82: // Keypad 0
            if (shift) return 0x5230;
            return 0x5200;
        case 83: // Keypad 1
            if (shift) return 0x4F31;
            return 0x4F00;
        case 84: // Keypad 2
            if (shift) return 0x5032;
            return 0x5000;
        case 85: // Keypad 3
            if (shift) return 0x5133;
            return 0x5100;
        case 86: // Keypad 4
            if (shift) return 0x4B34;
            return 0x4B00;
        case 87: // Keypad 5
            if (shift) return 0x4C35;
            if (ctrl) return 0x8F00;
            return 0x0000; // Center key has no normal scancode
        case 88: // Keypad 6
            if (shift) return 0x4D36;
            return 0x4D00;
        case 89: // Keypad 7
            if (shift) return 0x4737;
            return 0x4700;
        case 91: // Keypad 8
            if (shift) return 0x4838;
            return 0x4800;
        case 92: // Keypad 9
            if (shift) return 0x4939;
            return 0x4900;
        case 67: // Keypad *
            if (alt) return 0x3700;
            if (ctrl) return 0x9600;
            return 0x372A;
        case 69: // Keypad +
            if (alt) return 0x4E00;
            return 0x4E2B;
        case 65: // Keypad .
            if (shift) return 0x532E;
            return 0x5300;
        case 75: // Keypad /
            if (alt) return 0xA400;
            if (ctrl) return 0x9500;
            return 0x352F;
        case 76: // Keypad Enter
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
        case 81: // Keypad =
            return 0x0D3D; // Treat as regular equals
        case 78: // Keypad -
            if (alt) return 0x4A00;
            if (ctrl) return 0x8E00;
            return 0x4A2D;
            
        default:
            return 0x0000; // Unmapped key
    }
}

int get_statuscode(NSEvent *event) {
    NSUInteger flags = [event modifierFlags];
    int bios_byte = 0x00;

    // --- Shift Keys (Bits 0 & 1) ---
    // We use NX_ constants to distinguish Left from Right Shift
    if (flags & NX_DEVICERSHIFTKEYMASK) {
        bios_byte |= 0x01; // Bit 0: Right Shift
    }
    
    if (flags & NX_DEVICELSHIFTKEYMASK) {
        bios_byte |= 0x02; // Bit 1: Left Shift
    }

    // --- Control Key (Bit 2) ---
    // PC 0x417 usually treats generic Ctrl as Bit 2.
    // We check the high-level Cocoa flag for any Control key press.
    if (flags & NSEventModifierFlagControl) {
        bios_byte |= 0x04; // Bit 2: Ctrl
    }

    // --- Alt/Option Key (Bit 3) ---
    // Maps Mac 'Option' to PC 'Alt'
    if (flags & NSEventModifierFlagOption) {
        bios_byte |= 0x08; // Bit 3: Alt
    }

    // --- Scroll Lock (Bit 4) ---
    // Note: macOS keyboards rarely have a Scroll Lock, and NSEvent
    // does not consistently carry this state in modifierFlags.
    // This is usually left as 0 on Mac unless specifically handled by HID manager.
    // bios_byte |= 0x10; 

    // --- Num Lock (Bit 5) ---
    // NSEventModifierFlagNumericPad indicates a keypad key is pressed, 
    // but NOT that NumLock is active. Getting actual NumLock state requires
    // lower-level HID access rarely available in a simple NSEvent transform.
    // bios_byte |= 0x20;

    // --- Caps Lock (Bit 6) ---
    // Checks if Caps Lock is currently toggled On.
    if (flags & NSEventModifierFlagCapsLock) {
        bios_byte |= 0x40; // Bit 6: Caps Lock
    }

    // --- Insert (Bit 7) ---
    // macOS standard keyboards do not usually support 'Insert' toggle state.
    // Usually mapped to 'Help' or 'Fn' usage, rarely a toggle.
    // bios_byte |= 0x80;

    return bios_byte;
}