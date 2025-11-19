#ifndef _BIOS_H
#define _BIOS_H

/*
 * bioskey - Accesses BIOS keyboard services via Interrupt 0x16
 *
 * cmd values:
 * 0: Read character from keyboard buffer (waits if empty).
 * 1: Check if a keystroke is ready (returns 0 if empty, key value if ready).
 * 2: Get the current shift key state.
 *
 * Returns:
 * An integer representing the key value or status.
 * Lower 8 bits usually ASCII, Upper 8 bits scan code.
 */
int bioskey(int cmd);

#endif /* _BIOS_H */