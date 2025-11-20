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
    case 0:
        pccore.mode = CGA40x25;
        // its grey by default
        pccore.port[CGA_MODE_CONTROL_PORT] = 0x04; 
        break;    
    case 1:
        pccore.mode = CGA40x25;
        pccore.port[CGA_MODE_CONTROL_PORT] = 0x00; 
        break;             
    case 4:
        pccore.mode = CGA320x200x2;
        break;
    case 5:
        pccore.mode = CGA320x200x2g;
        // its grey by default
        pccore.port[CGA_MODE_CONTROL_PORT] = 0x4; 
        break;    
    case 6:
        pccore.mode = CGA640x200x1;
        break;          
    default:
        break;
    }
}