/**
 * @file linux_keyboard.c
 * @brief Converts X11 KeySyms to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "linux_keyboard.h"

/**
 * @brief Converts X11 KeySym to IBM PC 16-bit scan code
 * @param keysym The X11 KeySym value
 * @param state The modifier state from XKeyEvent
 * @return 16-bit IBM PC scan code (high byte = scan code, low byte = ASCII)
 *         Returns 0x0000 if the key is not mapped
 */
int get_scancode(KeySym keysym, unsigned int state) {
    // Extract modifier states
    int shift = (state & ShiftMask) != 0;
    int ctrl = (state & ControlMask) != 0;
    int alt = (state & Mod1Mask) != 0;
    
    // Letter keys (A-Z)
    switch (keysym) {
        // Letters
        case XK_a:
        case XK_A:
            if (alt) return 0x1E00;
            if (ctrl) return 0x1E01;
            if (shift) return 0x1E41;
            return 0x1E61;
            
        case XK_b:
        case XK_B:
            if (alt) return 0x3000;
            if (ctrl) return 0x3002;
            if (shift) return 0x3042;
            return 0x3062;
            
        case XK_c:
        case XK_C:
            if (alt) return 0x2E00;
            if (ctrl) return 0x2E03;
            if (shift) return 0x2E43;
            return 0x2E63;
            
        case XK_d:
        case XK_D:
            if (alt) return 0x2000;
            if (ctrl) return 0x2004;
            if (shift) return 0x2044;
            return 0x2064;
            
        case XK_e:
        case XK_E:
            if (alt) return 0x1200;
            if (ctrl) return 0x1205;
            if (shift) return 0x1245;
            return 0x1265;
            
        case XK_f:
        case XK_F:
            if (alt) return 0x2100;
            if (ctrl) return 0x2106;
            if (shift) return 0x2146;
            return 0x2166;
            
        case XK_g:
        case XK_G:
            if (alt) return 0x2200;
            if (ctrl) return 0x2207;
            if (shift) return 0x2247;
            return 0x2267;
            
        case XK_h:
        case XK_H:
            if (alt) return 0x2300;
            if (ctrl) return 0x2308;
            if (shift) return 0x2348;
            return 0x2368;
            
        case XK_i:
        case XK_I:
            if (alt) return 0x1700;
            if (ctrl) return 0x1709;
            if (shift) return 0x1749;
            return 0x1769;
            
        case XK_j:
        case XK_J:
            if (alt) return 0x2400;
            if (ctrl) return 0x240A;
            if (shift) return 0x244A;
            return 0x246A;
            
        case XK_k:
        case XK_K:
            if (alt) return 0x2500;
            if (ctrl) return 0x250B;
            if (shift) return 0x254B;
            return 0x256B;
            
        case XK_l:
        case XK_L:
            if (alt) return 0x2600;
            if (ctrl) return 0x260C;
            if (shift) return 0x264C;
            return 0x266C;
            
        case XK_m:
        case XK_M:
            if (alt) return 0x3200;
            if (ctrl) return 0x320D;
            if (shift) return 0x324D;
            return 0x326D;
            
        case XK_n:
        case XK_N:
            if (alt) return 0x3100;
            if (ctrl) return 0x310E;
            if (shift) return 0x314E;
            return 0x316E;
            
        case XK_o:
        case XK_O:
            if (alt) return 0x1800;
            if (ctrl) return 0x180F;
            if (shift) return 0x184F;
            return 0x186F;
            
        case XK_p:
        case XK_P:
            if (alt) return 0x1900;
            if (ctrl) return 0x1910;
            if (shift) return 0x1950;
            return 0x1970;
            
        case XK_q:
        case XK_Q:
            if (alt) return 0x1000;
            if (ctrl) return 0x1011;
            if (shift) return 0x1051;
            return 0x1071;
            
        case XK_r:
        case XK_R:
            if (alt) return 0x1300;
            if (ctrl) return 0x1312;
            if (shift) return 0x1352;
            return 0x1372;
            
        case XK_s:
        case XK_S:
            if (alt) return 0x1F00;
            if (ctrl) return 0x1F13;
            if (shift) return 0x1F53;
            return 0x1F73;
            
        case XK_t:
        case XK_T:
            if (alt) return 0x1400;
            if (ctrl) return 0x1414;
            if (shift) return 0x1454;
            return 0x1474;
            
        case XK_u:
        case XK_U:
            if (alt) return 0x1600;
            if (ctrl) return 0x1615;
            if (shift) return 0x1655;
            return 0x1675;
            
        case XK_v:
        case XK_V:
            if (alt) return 0x2F00;
            if (ctrl) return 0x2F16;
            if (shift) return 0x2F56;
            return 0x2F76;
            
        case XK_w:
        case XK_W:
            if (alt) return 0x1100;
            if (ctrl) return 0x1117;
            if (shift) return 0x1157;
            return 0x1177;
            
        case XK_x:
        case XK_X:
            if (alt) return 0x2D00;
            if (ctrl) return 0x2D18;
            if (shift) return 0x2D58;
            return 0x2D78;
            
        case XK_y:
        case XK_Y:
            if (alt) return 0x1500;
            if (ctrl) return 0x1519;
            if (shift) return 0x1559;
            return 0x1579;
            
        case XK_z:
        case XK_Z:
            if (alt) return 0x2C00;
            if (ctrl) return 0x2C1A;
            if (shift) return 0x2C5A;
            return 0x2C7A;
            
        // Number keys (1-0)
        case XK_1:
        case XK_exclam:
            if (alt) return 0x7800;
            if (shift) return 0x0221;
            return 0x0231;
            
        case XK_2:
        case XK_at:
            if (alt) return 0x7900;
            if (ctrl) return 0x0300;
            if (shift) return 0x0340; // @
            return 0x0332;
            
        case XK_3:
        case XK_numbersign:
            if (alt) return 0x7A00;
            if (shift) return 0x0423; // #
            return 0x0433;
            
        case XK_4:
        case XK_dollar:
            if (alt) return 0x7B00;
            if (shift) return 0x0524; // $
            return 0x0534;
            
        case XK_5:
        case XK_percent:
            if (alt) return 0x7C00;
            if (shift) return 0x0625; // %
            return 0x0635;
            
        case XK_6:
        case XK_asciicircum:
            if (alt) return 0x7D00;
            if (ctrl) return 0x071E;
            if (shift) return 0x075E; // ^
            return 0x0736;
            
        case XK_7:
        case XK_ampersand:
            if (alt) return 0x7E00;
            if (shift) return 0x0826; // &
            return 0x0837;
            
        case XK_8:
        case XK_asterisk:
            if (alt) return 0x7F00;
            if (shift) return 0x092A; // *
            return 0x0938;
            
        case XK_9:
        case XK_parenleft:
            if (alt) return 0x8000;
            if (shift) return 0x0A28; // (
            return 0x0A39;
            
        case XK_0:
        case XK_parenright:
            if (alt) return 0x8100;
            if (shift) return 0x0B29; // )
            return 0x0B30;
            
        // Symbol keys
        case XK_minus:
        case XK_underscore:
            if (alt) return 0x8200;
            if (ctrl) return 0x0C1F;
            if (shift) return 0x0C5F; // _
            return 0x0C2D;
            
        case XK_equal:
        case XK_plus:
            if (alt) return 0x8300;
            if (shift) return 0x0D2B; // +
            return 0x0D3D;
            
        case XK_bracketleft:
        case XK_braceleft:
            if (alt) return 0x1A00;
            if (ctrl) return 0x1A1B;
            if (shift) return 0x1A7B; // {
            return 0x1A5B;
            
        case XK_bracketright:
        case XK_braceright:
            if (alt) return 0x1B00;
            if (ctrl) return 0x1B1D;
            if (shift) return 0x1B7D; // }
            return 0x1B5D;
            
        case XK_semicolon:
        case XK_colon:
            if (alt) return 0x2700;
            if (shift) return 0x273A; // :
            return 0x273B;
            
        case XK_apostrophe:
        case XK_quotedbl:
            if (shift) return 0x2822; // "
            return 0x2827;
            
        case XK_grave:
        case XK_asciitilde:
            if (shift) return 0x297E; // ~
            return 0x2960;
            
        case XK_backslash:
        case XK_bar:
            if (alt) return 0x2600;
            if (ctrl) return 0x2B1C;
            if (shift) return 0x2B7C; // |
            return 0x2B5C;
            
        case XK_comma:
        case XK_less:
            if (shift) return 0x333C; // <
            return 0x332C;
            
        case XK_period:
        case XK_greater:
            if (shift) return 0x343E; // >
            return 0x342E;
            
        case XK_slash:
        case XK_question:
            if (shift) return 0x353F; // ?
            return 0x352F;
            
        // Function keys
        case XK_F1:
            if (alt) return 0x6800;
            if (ctrl) return 0x5E00;
            if (shift) return 0x5400;
            return 0x3B00;
            
        case XK_F2:
            if (alt) return 0x6900;
            if (ctrl) return 0x5F00;
            if (shift) return 0x5500;
            return 0x3C00;
            
        case XK_F3:
            if (alt) return 0x6A00;
            if (ctrl) return 0x6000;
            if (shift) return 0x5600;
            return 0x3D00;
            
        case XK_F4:
            if (alt) return 0x6B00;
            if (ctrl) return 0x6100;
            if (shift) return 0x5700;
            return 0x3E00;
            
        case XK_F5:
            if (alt) return 0x6C00;
            if (ctrl) return 0x6200;
            if (shift) return 0x5800;
            return 0x3F00;
            
        case XK_F6:
            if (alt) return 0x6D00;
            if (ctrl) return 0x6300;
            if (shift) return 0x5900;
            return 0x4000;
            
        case XK_F7:
            if (alt) return 0x6E00;
            if (ctrl) return 0x6400;
            if (shift) return 0x5A00;
            return 0x4100;
            
        case XK_F8:
            if (alt) return 0x6F00;
            if (ctrl) return 0x6500;
            if (shift) return 0x5B00;
            return 0x4200;
            
        case XK_F9:
            if (alt) return 0x7000;
            if (ctrl) return 0x6600;
            if (shift) return 0x5C00;
            return 0x4300;
            
        case XK_F10:
            if (alt) return 0x7100;
            if (ctrl) return 0x6700;
            if (shift) return 0x5D00;
            return 0x4400;
            
        case XK_F11:
            if (alt) return 0x8B00;
            if (ctrl) return 0x8900;
            if (shift) return 0x8700;
            return 0x8500;
            
        case XK_F12:
            if (alt) return 0x8C00;
            if (ctrl) return 0x8A00;
            if (shift) return 0x8800;
            return 0x8600;
            
        // Special keys
        case XK_BackSpace:
            if (alt) return 0x0E00;
            if (ctrl) return 0x0E7F;
            return 0x0E08;
            
        case XK_Delete:
            if (alt) return 0xA300;
            if (ctrl) return 0x9300;
            if (shift) return 0x532E;
            return 0x5300;
            
        case XK_Down:
            if (alt) return 0xA000;
            if (ctrl) return 0x9100;
            if (shift) return 0x5032;
            return 0x5000;
            
        case XK_End:
            if (alt) return 0x9F00;
            if (ctrl) return 0x7500;
            if (shift) return 0x4F31;
            return 0x4F00;
            
        case XK_Return:
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
            
        case XK_Escape:
            if (alt) return 0x0100;
            return 0x011B;
            
        case XK_Home:
            if (alt) return 0x9700;
            if (ctrl) return 0x7700;
            if (shift) return 0x4737;
            return 0x4700;
            
        case XK_Insert:
            if (alt) return 0xA200;
            if (ctrl) return 0x9200;
            if (shift) return 0x5230;
            return 0x5200;
            
        case XK_Left:
            if (alt) return 0x9B00;
            if (ctrl) return 0x7300;
            if (shift) return 0x4B34;
            return 0x4B00;
            
        case XK_Page_Down:
            if (alt) return 0xA100;
            if (ctrl) return 0x7600;
            if (shift) return 0x5133;
            return 0x5100;
            
        case XK_Page_Up:
            if (alt) return 0x9900;
            if (ctrl) return 0x8400;
            if (shift) return 0x4939;
            return 0x4900;
            
        case XK_Right:
            if (alt) return 0x9D00;
            if (ctrl) return 0x7400;
            if (shift) return 0x4D36;
            return 0x4D00;
            
        case XK_space:
            return 0x3920;
            
        case XK_Tab:
            if (alt) return 0xA500;
            if (ctrl) return 0x9400;
            if (shift) return 0x0F00;
            return 0x0F09;
            
        case XK_Up:
            if (alt) return 0x9800;
            if (ctrl) return 0x8D00;
            if (shift) return 0x4838;
            return 0x4800;
            
        // Keypad keys
        case XK_KP_0:
            if (shift) return 0x5230;
            return 0x5200;
            
        case XK_KP_1:
            if (shift) return 0x4F31;
            return 0x4F00;
            
        case XK_KP_2:
            if (shift) return 0x5032;
            return 0x5000;
            
        case XK_KP_3:
            if (shift) return 0x5133;
            return 0x5100;
            
        case XK_KP_4:
            if (shift) return 0x4B34;
            return 0x4B00;
            
        case XK_KP_5:
            if (shift) return 0x4C35;
            if (ctrl) return 0x8F00;
            return 0x0000;
            
        case XK_KP_6:
            if (shift) return 0x4D36;
            return 0x4D00;
            
        case XK_KP_7:
            if (shift) return 0x4737;
            return 0x4700;
            
        case XK_KP_8:
            if (shift) return 0x4838;
            return 0x4800;
            
        case XK_KP_9:
            if (shift) return 0x4939;
            return 0x4900;
            
        case XK_KP_Multiply:
            if (alt) return 0x3700;
            if (ctrl) return 0x9600;
            return 0x372A;
            
        case XK_KP_Add:
            if (alt) return 0x4E00;
            return 0x4E2B;
            
        case XK_KP_Decimal:
            if (shift) return 0x532E;
            return 0x5300;
            
        case XK_KP_Divide:
            if (alt) return 0xA400;
            if (ctrl) return 0x9500;
            return 0x352F;
            
        case XK_KP_Enter:
            if (alt) return 0xA600;
            if (ctrl) return 0x1C0A;
            return 0x1C0D;
            
        case XK_KP_Equal:
            return 0x0D3D;
            
        case XK_KP_Subtract:
            if (alt) return 0x4A00;
            if (ctrl) return 0x8E00;
            return 0x4A2D;
            
        default:
            return 0x0000; // Unmapped key
    }
}

/**
 * @brief Converts X11 modifier state to IBM PC BIOS keyboard status byte
 * @param state The modifier state from XKeyEvent
 * @return BIOS keyboard status byte (0x417)
 */
int get_statuscode(unsigned int state) {
    int bios_byte = 0x00;
    
    // Bit 0-1: Shift keys (X11 doesn't distinguish left/right easily)
    if (state & ShiftMask) {
        bios_byte |= 0x02; // Set left shift bit
    }
    
    // Bit 2: Control key
    if (state & ControlMask) {
        bios_byte |= 0x04;
    }
    
    // Bit 3: Alt key
    if (state & Mod1Mask) {
        bios_byte |= 0x08;
    }
    
    // Bit 4: Scroll Lock (Mod5 on some systems)
    if (state & Mod5Mask) {
        bios_byte |= 0x10;
    }
    
    // Bit 5: Num Lock (Mod2 on most systems)
    if (state & Mod2Mask) {
        bios_byte |= 0x20;
    }
    
    // Bit 6: Caps Lock
    if (state & LockMask) {
        bios_byte |= 0x40;
    }
    
    // Bit 7: Insert mode (not typically available in X11 state)
    // bios_byte |= 0x80;
    
    return bios_byte;
}