/**
 * @file linux_keyboard.h
 * @brief Converts X11 KeySyms to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#ifndef LINUX_KEYBOARD_H
#define LINUX_KEYBOARD_H

#include <X11/Xlib.h>
#include <X11/keysym.h>

/**
 * @brief Converts X11 KeySym to IBM PC 16-bit scan code
 * @param keysym The X11 KeySym value
 * @param state The modifier state from XKeyEvent
 * @return 16-bit IBM PC scan code (high byte = scan code, low byte = ASCII)
 *         Returns 0x0000 if the key is not mapped
 */
int get_scancode(KeySym keysym, unsigned int state);

/**
 * @brief Converts X11 modifier state to the IBM PC BIOS Data Area 
 * Keyboard Status Byte 1 (Memory Address 0x417).
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
 * @param state The X11 modifier state from XKeyEvent
 * @return An integer representing the 0x417 byte
 */
int get_statuscode(unsigned int state);

#endif // LINUX_KEYBOARD_H