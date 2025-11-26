#include "fire.h"
#include <stdlib.h> /* Required for rand(), srand() */
#include <time.h>   /* Required for time() */
/* Assuming dos.h and conio.h are handled by the compiler environment 
   or included within fire.h for the specific DOS functions (int86, MK_FP, etc.) */

/* Screen dimensions for CGA Mode 3 (80x25) */
#define COLS 80
#define ROWS 25

/* CGA Memory Pointer (0xB800:0000) */
unsigned char *cga_mem;

/* PALETTE DEFINITIONS 
   We use a lookup table for speed and to easily define complex color mixes.
*/

#define MAX_HEAT_INDEX 12

/* The 'heat' map now stores values from 0 to MAX_HEAT_INDEX.
   0 is cold, 12 is white-hot.
*/
unsigned char heat_map[ROWS][COLS]; 

/* Lookup Table: Characters 
   Maps a heat value (index) to an ASCII character.
*/
const unsigned char FIRE_CHARS[] = {
    ' ',   /* 0:  Cold (Black) */
    0xB0,  /* 1:  Dim Red (Light Shade) */
    0xB1,  /* 2:  Red (Medium Shade) */
    0xB2,  /* 3:  Bright Red (Dark Shade) */
    0xDB,  /* 4:  Solid Red */
    0xB0,  /* 5:  Magma Start (Red BG + Yellow FG, Light Shade) */
    0xB1,  /* 6:  Magma Mid (Red BG + Yellow FG, Medium Shade) */
    0xB2,  /* 7:  Magma Hot (Red BG + Yellow FG, Dark Shade) */
    0xDB,  /* 8:  Solid Yellow */
    0xB1,  /* 9:  White Hot (Brown BG + White FG, Medium Shade) */
    0xB2,  /* 10: White Hot (Brown BG + White FG, Dark Shade) */
    0xB2,  /* 11: Max Heat (Keeping texture) */
    0xDB   /* 12: Pure White */
};

/* Lookup Table: Attributes (Colors) */
const unsigned char FIRE_ATTRS[] = {
    0x00,             /* 0:  Black BG, Black FG */
    0x04,             /* 1:  Black BG, Red FG */
    0x04,             /* 2:  Black BG, Red FG */
    0x04,             /* 3:  Black BG, Red FG */
    0x04,             /* 4:  Black BG, Red FG */
    (0x4 << 4) | 0xE, /* 5:  Red BG,   Yellow FG (Mix = Orange) */
    (0x4 << 4) | 0xE, /* 6:  Red BG,   Yellow FG */
    (0x4 << 4) | 0xE, /* 7:  Red BG,   Yellow FG */
    0x0E,             /* 8:  Black BG, Yellow FG */
    (0x6 << 4) | 0xF, /* 9:  Brown BG, White FG (Mix = Pale Yellow) */
    (0x6 << 4) | 0xF, /* 10: Brown BG, White FG */
    (0x6 << 4) | 0xF, /* 11: Brown BG, White FG */
    0x0F              /* 12: Black BG, White FG */
};

void clear_screen(void) {
    int i, r, c;
    /* 1. Clear CGA Memory */
    for (i = 0; i < COLS * ROWS * 2; i += 2) {
        cga_mem[i] = ' '; 
        cga_mem[i + 1] = 0x00;
    }

    /* 2. Clear Heat Map */
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            heat_map[r][c] = 0;
        }
    }
}

void render_fire(void) {
    int r, c, offset, avg_heat;
    int rand_decay;
    int spark;        /* C90: Must be declared at start of block */
    unsigned char heat; /* C90: Must be declared at start of block */
    
    /* 1. Generate Heat at the Bottom Row */
    r = ROWS - 1; 
    for (c = 0; c < COLS; c++) {
        /* Generate heat. */
        spark = rand() % 100;
        
        if (spark > 40) {
            /* Hot spots */
            heat_map[r][c] = MAX_HEAT_INDEX; 
        } else if (spark > 10) {
            /* Medium burn */
            heat_map[r][c] = MAX_HEAT_INDEX - 2; 
        } else {
            /* Occasional dips for flicker effect */
            heat_map[r][c] = 0;
        }
    }

    /* 2. Propagate and Cool */
    for (r = 0; r < ROWS - 1; r++) { 
        for (c = 0; c < COLS; c++) {
            
            /* Sample from pixels below */
            avg_heat = heat_map[r+1][c]; 
            
            if (c > 0)          avg_heat += heat_map[r+1][c-1];
            else                avg_heat += heat_map[r+1][c];
            
            if (c < COLS - 1)   avg_heat += heat_map[r+1][c+1];
            else                avg_heat += heat_map[r+1][c];
            
            if (r + 2 < ROWS)   avg_heat += heat_map[r+2][c];
            else                avg_heat += heat_map[r+1][c];

            /* Average of 4 samples */
            avg_heat /= 4; 

            /* Decay: Slightly different logic to preserve the longer palette.
               Randomly subtract 0 or 1. */
            if (avg_heat > 0) {
                rand_decay = rand() % 4; /* 0, 1, 2, 3 */
                if (rand_decay > 0) {
                    avg_heat -= 1;
                }
            }

            /* Clamp */
            if (avg_heat < 0) avg_heat = 0;
            if (avg_heat > MAX_HEAT_INDEX) avg_heat = MAX_HEAT_INDEX;

            heat_map[r][c] = (unsigned char)avg_heat;
        }
    }
    
    /* 3. Render to CGA Memory using Lookup Tables */
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            offset = (r * COLS + c) * 2; 
            heat = heat_map[r][c];
            
            /* Safety clamp just in case */
            if (heat > MAX_HEAT_INDEX) heat = MAX_HEAT_INDEX;
            
            /* Direct lookup */
            cga_mem[offset] = FIRE_CHARS[heat];
            cga_mem[offset + 1] = FIRE_ATTRS[heat];
        }
    }
}

int dos_main(int argc, char *argv[])
{
    union REGS regs;

    cga_mem = MK_FP(0xB800, 0x0000);

    /* Set Mode 3 */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs);

    srand(time(NULL));
    clear_screen();

    while (!kbhit()) {
        render_fire();
        /* Removed delay or keep it very small for smoother 60fps look */
        delay(100); 
    }

    /* Restore text mode before exiting to clear colors */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs);

    return 0;
}