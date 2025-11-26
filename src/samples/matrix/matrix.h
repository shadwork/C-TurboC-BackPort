#ifndef MATRIX_H
#define MATRIX_H

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

int dos_main(int argc, char *argv[]);

#endif