#include "conio.h"
#include "bios.h" // Assuming bios.h is in the same directory (../turboc/)

/*
 * kbhit - Checks for available keystrokes in the keyboard buffer.
 *
 * Implements the conio.h kbhit function by calling the BIOS keyboard
 * service check (cmd=1).
 */
int kbhit(void) {
    return bioskey(1);
}