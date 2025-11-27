# Disclaimer
Borland Turbo C IDE is somewhat outdated by modern IDE standards. The main goal of the project is to develop Turbo C code in a modern IDE with the ability to run the code directly on modern OSes with CGA-style graphics. After building, your app starts a window that emulates the CGA video adapter and supports scaling and fullscreen.

# How to build
Project compiled with CMAKE running from src folder. You should install cmake properly fro you OS.

    cd src
    cmake .
    make
And running a couple of demos from samples folder, for example

    ./bin/plasma

# Project structure
You can write DOS code in two ways:
 - [single c file](/src/templates/single/single.c)
 - [multiple c files](/src/templates/multiple/multiple.c)

 The main difference is prj file for using in Turbo C