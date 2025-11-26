#include "fire.h"

/* Screen dimensions for CGA Mode 3 (80x25) */
#define COLS 80
#define ROWS 25
#define CGA_BUFFER_SIZE (COLS * ROWS * 2) /* 4000 bytes */

/* CGA Memory Pointer (0xB800:0000) */
unsigned char *cga_mem;

/* Fire Attributes and Characters */
/* Background is always Black (0x0) */
#define BLACK_BG    0x0
#define CHAR_SOLID  0xDB /* ASCII code for a solid block */
#define CHAR_FADE   0xB0 /* ASCII code for a light shade/speckle */
#define CHAR_EMPTY  ' '  /* Space character */

/* Foreground colors for the fire effect (Background is implicitly Black) */
#define FIRE_RED        0x4 /* Red foreground */
#define FIRE_LIGHT_RED  0xC /* Bright Red foreground */
#define FIRE_YELLOW     0xE /* Bright Yellow foreground */
#define FIRE_ORANGE_1   0x6 /* Brown/Orange foreground (Standard Yellow) */

/* The 'heat' or 'color' of a cell, 0 is cold (black), higher is hotter/brighter */
unsigned char heat_map[ROWS][COLS]; 

/* Function to clear the screen attribute and heat map */
void clear_screen() {
    int i, r, c;
    /* 1. Clear CGA Memory */
    /* 80 columns * 25 rows * 2 bytes/cell = 4000 bytes */
    for (i = 0; i < COLS * ROWS * 2; i += 2) {
        cga_mem[i] = CHAR_EMPTY;  /* Character space */
        cga_mem[i + 1] = BLACK_BG;/* Attribute black */
    }

    /* 2. Clear Heat Map */
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            heat_map[r][c] = 0;
        }
    }
}

/* Function to map a heat value (0-15) to a CGA attribute byte */
unsigned char heat_to_attribute(unsigned char heat) {
    /* Heat 0 is cold (Black BG, Black FG - implicit 0x0) */
    if (heat == 0) return BLACK_BG;
    
    /* Simulate the fade from yellow/white hot to red to black */
    if (heat >= 13) return FIRE_YELLOW;    /* Hottest/Brightest */
    if (heat >= 10) return FIRE_LIGHT_RED;
    if (heat >= 7)  return FIRE_RED;
    if (heat >= 4)  return FIRE_ORANGE_1;
    
    return BLACK_BG; /* Fading out / cooler */
}

/* Function to map a heat value to a display character */
unsigned char heat_to_char(unsigned char heat) {
    if (heat >= 14) return CHAR_SOLID; /* Solid block for the hottest core */
    if (heat >= 10) return CHAR_FADE;  /* Speckled/light shade for the main flame */
    return CHAR_EMPTY;                /* Space for the cooler, fading edges */
}


/* Function to render one frame of the fire */
void render_fire() {
    int r, c, offset, avg_heat;
    
    /* 1. Generate Heat at the Bottom Row (Source of Fire) */
    r = ROWS - 1; /* Last row (bottom of the screen) */
    for (c = 0; c < COLS; c++) {
        /* Introduce random high heat (10 to 15) to simulate a flame source */
        heat_map[r][c] = (unsigned char)(rand() % 6 + 10); 
    }

    /* 2. Propagate and Cool the Heat (The Fire Simulation) */
    for (r = 0; r < ROWS - 1; r++) { /* Start from the top row and go down to the second-to-last */
        for (c = 0; c < COLS; c++) {
            
            /* Calculate the average heat of the cell below and its neighbors. 
               This simulates the heat rising and spreading. */
            
            /* The cell below (r+1, c) */
            avg_heat = heat_map[r+1][c] * 2; 

            /* The bottom-left neighbor (r+1, c-1). Check boundary. */
            if (c > 0) {
                avg_heat += heat_map[r+1][c-1];
            } else {
                avg_heat += heat_map[r+1][c]; /* Wrap/mirror or use current for boundary */
            }

            /* The bottom-right neighbor (r+1, c+1). Check boundary. */
            if (c < COLS - 1) {
                avg_heat += heat_map[r+1][c+1];
            } else {
                avg_heat += heat_map[r+1][c]; /* Wrap/mirror or use current for boundary */
            }
            
            /* Divide by the number of samples (4 in total) to get the average. */
            avg_heat /= 4; 

            /* Cooling: Subtract a random small value (1 or 2) to simulate heat dissipation */
            avg_heat -= (rand() % 2 + 1);

            /* Clamp the heat value to ensure it's not negative */
            if (avg_heat < 0) avg_heat = 0;

            /* Apply the new heat value to the current cell */
            heat_map[r][c] = (unsigned char)avg_heat;
        }
    }
    
    /* 3. Render the Heat Map to the CGA Memory */
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            offset = (r * COLS + c) * 2; 
            
            /* Set the character and attribute based on the calculated heat */
            cga_mem[offset] = heat_to_char(heat_map[r][c]);
            cga_mem[offset + 1] = heat_to_attribute(heat_map[r][c]);
        }
    }
}

int dos_main(int argc, char *argv[])
{
    union REGS regs;

    /* 1. Set CGA Memory Pointer */
    /* MK_FP creates a far pointer from segment (0xB800) and offset (0x0000) */
    cga_mem = MK_FP(0xB800, 0x0000);

    /* 2. Set Color Text Mode (Mode 3: 80x25 color text) */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs); /* BIOS video interrupt */

    /* 3. Initialize Randomizer and Data */
    srand(time(NULL));
    clear_screen();

    /* 4. Main Rendering Loop */
    while (!kbhit()) { /* Loop until a key is pressed */
        render_fire();
        delay(100); /* Adjust for effect speed */
    }

    return 0;
}