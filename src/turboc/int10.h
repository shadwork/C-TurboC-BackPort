#ifndef INT10_H
#define INT10_H

#include "dos.h"
#include "../pccore/pccore.h"
#include "../pccore/cga.h"

/**
 * @brief Emulates the BIOS Interrupt 0x10 (Video Services).
 * * @param inregs  Pointer to input registers containing arguments (AH = function).
 * @param outregs Pointer to output registers for results.
 * @return        The value of AX after the interrupt.
 */
int int10(union REGS *inregs, union REGS *outregs);

void setVideoMode(int mode);

#endif /* INT10_H */