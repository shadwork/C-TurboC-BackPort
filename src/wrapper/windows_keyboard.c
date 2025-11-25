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

/**
 * @brief Converts Windows virtual key code to IBM PC 16-bit scan code
 */
int get_scancode(WPARAM vkCode, LPARAM lParam, BYTE *keyboardState) {
    // Extract modifier states
    BOOL shift = (keyboardState[VK_SHIFT] & 0x80) != 0;
    BOOL ctrl = (keyboardState[VK_CONTROL] & 0x80) != 0;
    BOOL alt = (keyboardState[VK_MENU] & 0x80) != 0;
    
    // Letter keys (A-Z)
    switch (vkCode) {
        // Letters
        case 'A':
            if (alt) return 0x1E00;
            if (ctrl) return 0x1E01;
            if (shift) return 0x1E41;
            return 0x1E61;
        case 'B':
            if (alt) return 0x3000;
            if (ctrl) return 0x3002;
            if (shift) return 0x3042;
            return 0x3062;
        case 'C':
            if (alt) return 0x2E00;
            if (ctrl) return 0x2E03;
            if (shift) return 0x2E43;
            return 0x2E63;
        case 'D':
            if (alt) return 0x2000;
            if (ctrl) return 0x2004;
            if (shift) return 0x2044;
            return 0x2064;
        case 'E':
            if (alt) return 0x1200;
            if (ctrl) return 0x1205;
            if (shift) return 0x1245;
            return 0x1265;
        case 'F':
            if (alt) return 0x2100;
            if (ctrl) return 0x2106;
            if (shift) return 0x2146;
            return 0x2166;
        case 'G':
            if (alt) return 0x2200;
            if (ctrl) return 0x2207;
            if (shift) return 0x2247;
            return 0x2267;
        case 'H':
            if (alt) return 0x2300;
            if (ctrl) return 0x2308;
            if (shift) return 0x2348;
            return 0x2368;
        case 'I':
            if (alt) return 0x1700;
            if (ctrl) return 0x1709;
            if (shift) return 0x1749;
            return 0x1769;
        case 'J':
            if (alt) return 0x2400;
            if (ctrl) return 0x240A;
            if (shift) return 0x244A;
            return 0x246A;
        case 'K':
            if (alt) return 0x2500;
            if (ctrl) return 0x250B;
            if (shift) return 0x254B;
            return 0x256B;
        case 'L':
            if (alt) return 0x2600;
            if (ctrl) return 0x260C;
            if (shift) return 0x264C;
            return 0x266C;
        case 'M':
            if (alt) return 0x3200;
            if (ctrl) return 0x320D;
            if (shift) return 0x324D;
            return 0x326D;
        case 'N':
            if (alt) return 0x3100;
            if (ctrl) return 0x310E;
            if (shift) return 0x314E;
            return 0x316E;
        case 'O':
            if (alt) return 0x1800;
            if (ctrl) return 0x180F;
            if (shift) return 0x184F;
            return 0x186F;
        case 'P':
            if (alt) return 0x1900;
            if (ctrl) return 0x1910;
            if (shift) return 0x1950;
            return 0x1970;
        case 'Q':
            if (alt) return 0x1000;
            if (ctrl) return 0x1011;
            if (shift) return 0x1051;
            return 0x1071;
        case 'R':
            if (alt) return 0x1300;
            if (ctrl) return 0x1312;
            if (shift) return 0x1352;
            return 0x1372;
        case 'S':
            if (alt) return 0x1F00;
            if (ctrl) return 0x1F13;
            if (shift) return 0x1F53;
            return 0x1F73;
        case 'T':
            if (alt) return 0x1400;
            if (ctrl) return 0x1414;
            if (shift) return 0x1454;
            return 0x1474;
        case 'U':
            if (alt) return 0x1600;
            if (ctrl) return 0x1615;
            if (shift) return 0x1655;
            return 0x1675;
        case 'V':
            if (alt) return 0x2F00;
            if (ctrl) return 0x2F16;
            if (shift) return 0x2F56;
            return 0x2F76;
        case 'W':
            if (alt) return 0x1100;
            if (ctrl) return 0x1117;
            if (shift) return 0x1157;
            return 0x1177;
        case 'X':
            if (alt) return 0x2D00;
            if (ctrl) return 0x2D18;
            if (shift) return 0x2D58;
            return 0x2D78;
        case 'Y':
            if (alt) return 0x1500;
            if (ctrl) return 0x1519;
            if (shift) return 0x1559;
            return 0x1579;
        case 'Z':
            if (alt) return 0x2C00;
            if (ctrl) return 0x2C1A;
            if (shift) return 0x2C5A;
            return 0x2C7A;
            
        // Number keys (1-0)
        case '1':
            if (alt) return 0x7800;
            if (shift) return 0x0221; // !
            return 0x0231;
        case '2':
            if (alt) return 0x7900;
            if (ctrl) return 0x0300;
            if (shift) return 0x0340; // @
            return 0x0332;
        case '3':
            if (alt) return 0x7A00;
            if (shift) return 0x0423; // #
            return 0x0433;
        case '4':
            if (alt) return 0x7B00;
            if (shift) return 0x0524; // $
            return 0x0534;
        case '5':
            if (alt) return 0x7C00;
            if (shift) return 0x0625; // %
            return 0x0635;
        case '6':
            if (alt) return 0x7D00;
            if (ctrl) return 0x071E;
            if (shift) return 0x075E; // ^
            return 0x0736;
        case '7':
            if (alt) return 0x7E00;
            if (shift) return 0x0826; // &
            return 0x0837;
        case '8':
            if (alt) return 0x7F00;
            if (shift) return 0x092A; // *
            return 0x0938;
        case '9':
            if (alt) return 0x8000;
            if (shift) return 0x0A28; // (
            return 0x0A39;
        case '0':
            if (alt) return 0x8100;
            if (shift) return 0x0B29; // )
            return 0x0B30;
            
        // Symbol keys
        case VK_OEM_MINUS: // - (minus/underscore)
            if (alt) return 0x8200;
            if (ctrl) return 0x0C1F;
            if (shift) return 0x0C5F; // _
            return 0x0C2D;
        case VK_OEM_PLUS: // = (equals/plus)
            if (alt) return 0x8300;
            if (shift) return 0x0D2B; // +
            return 0x0D3D;
        case VK_OEM_4: // [ (left bracket)
            if (alt) return 0x1A00;
            if (ctrl) return 0x1A1B;
            if (shift) return 0x1A7B; // {
            return 0x1A5B;
        case VK_OEM_6: // ] (right bracket)
            if (alt) return 0x1B00;
            if (ctrl) return 0x1B1D;
            if (shift) return 0x1B7D; // }
            return 0x1B5D;
        case VK_OEM_1: // ; (semicolon)
            if (alt) return 0x2700;
            if (shift) return 0x273A; // :
            return 0x273B;
        case VK_OEM_7: // ' (apostrophe)
            if (shift) return 0x2822; // "
            return 0x2827;
        case VK_OEM_3: // ` (grave accent/tilde)
            if (shift) return 0x297E; // ~
            return 0x2960;
        case VK_OEM_5: // \ (backslash)
            if (alt) return 0x2600;
            if (ctrl) return 0x2B1C;
            if (shift) return 0x2B7C; // |
            return 0x2B5C;
        case VK_OEM_COMMA: // , (comma)
            if (shift) return 0x333C; // <
            return 0x332C;
        case VK_OEM_PERIOD: // . (period)
            if (shift) return 0x343E; // >
            return 0x342E;
        case VK_OEM_2: // / (slash)
            if (shift) return 0x353F; // ?
            return 0x352F;
            
        // Function keys
        case VK_F1:
            if (alt) return 0x6800;
            if (ctrl) return 0x5E00;
            if (shift) return 0x5400;
            return 0x3B00;
        case VK_F2:
            if (alt) return 0x6900;
            if (ctrl) return 0x5F00;
            if (shift) return 0x5500;
            return 0x3C00;
        case VK_F3:
            if (alt) return 0x6A00;
            if (ctrl) return 0x6000;
            if (shift) return 0x5600;
            return 0x3D00;
        case VK_F4:
            if (alt) return 0x6B00;
            if (ctrl) return 0x6100;
            if (shift) return 0x5700;
            return 0x3E00;
        case VK_F5:
            if (alt) return 0x6C00;
            if (ctrl) return 0x6200;
            if (shift) return 0x5800;
            return 0x3F00;
        case VK_F6:
            if (alt) return 0x6D00;
            if (ctrl) return 0x6300;
            if (shift) return 0x5900;
            return 0x4000;
        case VK_F7:
            if (alt) return 0x6E00;
            if (ctrl) return 0x6400;
            if (shift) return 0x5A00;
            return 0x4100;
        case VK_F8:
            if (alt) return 0x6F00;
            if (ctrl) return 0x6500;
            if (shift) return 0x5B00;
            return 0x4200;
        case VK_F9:
            if (alt) return 0x7000;
            if (ctrl) return 0x6600;
            if (shift) return 0x5C00;
            return 0x4300;
        case VK_F10:
            if (alt) return 0x7100;
            if (ctrl) return 0x6700;
            if (shift) return 0x5D00;
            return 0x4400;
        case VK_F11:
            if (alt) return 0x8B00;
            if (ctrl) return 0x8900;
            if (shift) return 0x8700;
            return 0x8500;
        case VK_F12:
            if (alt) return 0x8C00;
            if (ctrl) return 0x8A00;
            if (shift) return 0x8800;
            return 0x8600;
            
        // Special keys
        case VK_BACK: // Backspace
            if (alt) return 0x0E00;
            if (ctrl) return 0x0E7F;
            return 0x0E08;
        case VK_DELETE:
            if (alt) return 0xA300;
            if (ctrl) return 0x9300;
            if (shift) return 0x532E;
            return 0x5300;
        case VK_DOWN: // Down Arrow
            if (alt) return 0xA000;
            if (ctrl) return 0x9100;
            if (shift) return 0x5032;
            return 0x5000;
        case VK_END:
            if (alt) return 0x9F00;
            if (ctrl) return 0x7500;
            if (shift) return 0x4F31;
            return 0x4F00;
        case VK_RETURN: // Enter
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
        case VK_LEFT: // Left Arrow
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
        case VK_RIGHT: // Right Arrow
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
        case VK_UP: // Up Arrow
            if (alt) return 0x9800;
            if (ctrl) return 0x8D00;
            if (shift) return 0x4838;
            return 0x4800;
            
        // Numeric keypad
        case VK_NUMPAD0:
            if (shift) return 0x5230;
            return 0x5200;
        case VK_NUMPAD1:
            if (shift) return 0x4F31;
            return 0x4F00;
        case VK_NUMPAD2:
            if (shift) return 0x5032;
            return 0x5000;
        case VK_NUMPAD3:
            if (shift) return 0x5133;
            return 0x5100;
        case VK_NUMPAD4:
            if (shift) return 0x4B34;
            return 0x4B00;
        case VK_NUMPAD5:
            if (shift) return 0x4C35;
            if (ctrl) return 0x8F00;
            return 0x0000; // Center key has no normal scancode
        case VK_NUMPAD6:
            if (shift) return 0x4D36;
            return 0x4D00;
        case VK_NUMPAD7:
            if (shift) return 0x4737;
            return 0x4700;
        case VK_NUMPAD8:
            if (shift) return 0x4838;
            return 0x4800;
        case VK_NUMPAD9:
            if (shift) return 0x4939;
            return 0x4900;
        case VK_MULTIPLY: // Keypad *
            if (alt) return 0x3700;
            if (ctrl) return 0x9600;
            return 0x372A;
        case VK_ADD: // Keypad +
            if (alt) return 0x4E00;
            return 0x4E2B;
        case VK_DECIMAL: // Keypad .
            if (shift) return 0x532E;
            return 0x5300;
        case VK_DIVIDE: // Keypad /
            if (alt) return 0xA400;
            if (ctrl) return 0x9500;
            return 0x352F;
        case VK_SUBTRACT: // Keypad -
            if (alt) return 0x4A00;
            if (ctrl) return 0x8E00;
            return 0x4A2D;
            
        default:
            return 0x0000; // Unmapped key
    }
}

/**
 * @brief Converts Windows keyboard state to IBM PC BIOS status byte
 */
int get_statuscode(BYTE *keyboardState) {
    int bios_byte = 0x00;

    // Bit 0: Right Shift
    if (keyboardState[VK_RSHIFT] & 0x80) {
        bios_byte |= 0x01;
    }

    // Bit 1: Left Shift
    if (keyboardState[VK_LSHIFT] & 0x80) {
        bios_byte |= 0x02;
    }

    // Bit 2: Ctrl (either)
    if ((keyboardState[VK_CONTROL] & 0x80) || 
        (keyboardState[VK_LCONTROL] & 0x80) || 
        (keyboardState[VK_RCONTROL] & 0x80)) {
        bios_byte |= 0x04;
    }

    // Bit 3: Alt (either)
    if ((keyboardState[VK_MENU] & 0x80) || 
        (keyboardState[VK_LMENU] & 0x80) || 
        (keyboardState[VK_RMENU] & 0x80)) {
        bios_byte |= 0x08;
    }

    // Bit 4: Scroll Lock
    if (keyboardState[VK_SCROLL] & 0x01) {
        bios_byte |= 0x10;
    }

    // Bit 5: Num Lock
    if (keyboardState[VK_NUMLOCK] & 0x01) {
        bios_byte |= 0x20;
    }

    // Bit 6: Caps Lock
    if (keyboardState[VK_CAPITAL] & 0x01) {
        bios_byte |= 0x40;
    }

    // Bit 7: Insert
    if (keyboardState[VK_INSERT] & 0x01) {
        bios_byte |= 0x80;
    }

    return bios_byte;
}