#include "dosapp.h"
#include <stdio.h> /* For printf */

#include "turboc/dos.h"
#include "turboc/bios.h"

/*
 * dosapp.c
 * C90 compatible source file.
 * Contains the real 'main' entry point and
 * the implementation of the 'dos_main' function.
 */

/*
 * Implementation of the mock main function.
 * This is where the application's primary logic would go.
 */
int dos_main(int argc, char *argv[])
{
    int i; /* C90 requires variables to be declared at the top of a block */
    unsigned char *cga_mem;
    union REGS regs;
    int inkey;

    cga_mem = MK_FP(0xB800,0000);
    cga_mem[0] = 255;

    regs.h.ah = 0;
    regs.h.al = 4;
    int86(0x10,&regs,&regs);

    outportb(0x3d9,0);

    do{
        while(bioskey (1)==0);
        inkey = bioskey (0);
        cga_mem[0] = inkey & 0xf;
        cga_mem[2] = inkey >> 8;

    }while(inkey != 0x011b);

    printf("--- Inside dos_main (the 'mock' main) ---\n");
    printf("Argument count: %d\n", argc);

    for (i = 0; i < argc; ++i)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    printf("--- Exiting dos_main ---\n");

    /* Return 0 to indicate success */
    return 0;
}