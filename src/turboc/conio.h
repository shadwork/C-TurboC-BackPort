#ifndef _CONIO_H
#define _CONIO_H

/*
 * kbhit - Checks for keyboard hit
 *
 * Returns a non-zero value if a key has been pressed and is waiting in the
 * keyboard buffer. Returns 0 if the buffer is empty.
 *
 * This function typically wraps bioskey(1).
 */
int kbhit(void);

#endif /* _CONIO_H */