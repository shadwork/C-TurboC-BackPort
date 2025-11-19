#include "int10.h"
#include <string.h> 

/**
 * @brief Handler for INT 10h (Video BIOS Services).
 * * Dispatches commands based on the value in AH.
 */
int int10(union REGS *inregs, union REGS *outregs)
{
    /* Standard behavior: Copy input to output, then modify output as needed */
    if (inregs != outregs) {
        memcpy(outregs, inregs, sizeof(union REGS));
    }

    /* Dispatch based on AH (Function ID) */
    switch (inregs->h.ah) {
        
        /* Function 00h: Set Video Mode */
        case 0x00:
            setVideoMode(inregs->h.al);
            break;

        default:
            /* Unimplemented function */
            break;
    }

    return outregs->x.ax;
}

void setVideoMode(int mode){
    pccore.port[CGA_COLOR_REGISTER_PORT] = 0;
    memset(&pccore.memory[CGA_VIDEO_RAM_START],0,CGA_BANK1_OFFSET*2);
    switch (mode)
    {
    case 4:
        pccore.mode = CGA320x200x2;
        break;
    
    default:
        break;
    }
}