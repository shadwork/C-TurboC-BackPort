/* Original includes needed for Turbo C/DOS environment */
#ifndef WRAPPER
#include <dos.h>
int main(int argc, char* argv[]){return dos_main(argc,argv);}
#else
#include "../turboc/dos.h"
#endif

#include <stdlib.h>

/* Write you code here */
int dos_main(int argc, char *argv[]){
    return 0;
};