/**
 * @file windows_keyboard.h
 * @brief Converts Windows virtual key codes to IBM PC 16-bit scan codes
 * 
 * IBM PC Scan Code Format (16-bit):
 * - High byte: Scan code (hardware key position)
 * - Low byte: ASCII character code
 * 
 * Reference: https://wiki.nox-rhea.org/back2root/ibm-pc-ms-dos/hardware/informations/keyboard-scan-code
 */

#ifndef WINDOWS_KEYBOARD_H
#define WINDOWS_KEYBOARD_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts Windows virtual key code to IBM PC 16-bit scan code
 */
int get_scancode(WPARAM vkCode, LPARAM lParam);

/**
 * @brief Gets the IBM PC BIOS keyboard status byte
 */
int get_statuscode(void);

#ifdef __cplusplus
}
#endif

#endif // WINDOWS_KEYBOARD_H
