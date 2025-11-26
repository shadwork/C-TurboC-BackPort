#ifndef FIRE_H
#define FIRE_H

/* Original includes needed for Turbo C/DOS environment */
#ifndef WRAPPER
#include <dos.h>
#include <bios.h>
#include <conio.h>
#else
#include "../turboc/dos.h"
#include "../turboc/bios.h"
#include "../turboc/conio.h"
#include "../turboc/time.h"
#endif

#include <stdlib.h>

/* The entry point for the DOS program */
int dos_main(int argc, char *argv[]);

#endif