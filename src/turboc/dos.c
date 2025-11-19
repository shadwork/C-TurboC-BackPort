#include "dos.h"
#include "../pccore/pccore.h"
#include "int10.h"

int int86(int intno,union REGS *inregs, union REGS *outregs)
{
    switch (intno)
    {
    case 0x10:
        return int10(inregs,outregs);
        break;
    default:
        break;
    }
    return 0;
}

void outportb(int portid, char value){
    pccore.port[portid] = value;
}

void* MK_FP(int seg, int ofs)
{
    unsigned long linear_address = (unsigned long)(seg * 16) + (unsigned long)ofs;
    return (void*)&pccore.memory[linear_address];
}