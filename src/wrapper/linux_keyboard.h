/**
 * @file linux_keyboard.h
 * @brief Converts Linux GDK key codes to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#ifndef LINUX_KEYBOARD_H
#define LINUX_KEYBOARD_H

#include <gdk/gdk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts GDK key event to IBM PC 16-bit scan code
 * @param event The GdkEventKey containing key code and modifier flags
 * @return 16-bit IBM PC scan code (high byte = scan code, low byte = ASCII)
 *         Returns 0x0000 if the key is not mapped
 */
int get_scancode(GdkEventKey *event);

/**
 * @brief Converts GDK key event modifier flags to IBM PC BIOS Data Area 
 *        Keyboard Status Byte 1 (Memory Address 0x417).
 *
 * 0x417 Layout:
 * Bit 7: Insert active
 * Bit 6: Caps Lock active
 * Bit 5: Num Lock active
 * Bit 4: Scroll Lock active
 * Bit 3: Alt key pressed
 * Bit 2: Ctrl key pressed
 * Bit 1: Left Shift pressed
 * Bit 0: Right Shift pressed
 *
 * @param event The GdkEventKey to analyze
 * @return An integer representing the 0x417 byte
 */
int get_statuscode(GdkEventKey *event);

#ifdef __cplusplus
}
#endif

#endif // LINUX_KEYBOARD_H
