#include "plasma.h"

/* Screen dimensions for CGA Mode 3 (80x25) */
#define COLS 80
#define ROWS 25

/* Constants */
#define PALETTE_SIZE 128
#define SINE_SIZE 256

/* CGA Memory Pointer (0xB800:0000) */
unsigned char *cga_mem;

/* Lookup Tables */
unsigned char palette_char[PALETTE_SIZE];
unsigned char palette_attr[PALETTE_SIZE];

/* Hardcoded Sine Table (Amplitude 32, Period 128, Repeated 2x for 256 entries) */
const int sine_table[SINE_SIZE] = {
    0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16, 17, 19, 20, 21, 
    22, 23, 24, 25, 26, 27, 28, 28, 29, 29, 30, 30, 30, 31, 31, 31, 
    32, 31, 31, 31, 30, 30, 30, 29, 29, 28, 28, 27, 26, 25, 24, 23, 
    22, 21, 20, 19, 17, 16, 15, 13, 12, 10, 9, 7, 6, 4, 3, 1, 
    0, -1, -3, -4, -6, -7, -9, -10, -12, -13, -15, -16, -17, -19, -20, -21, 
    -22, -23, -24, -25, -26, -27, -28, -28, -29, -29, -30, -30, -30, -31, -31, -31, 
    -32, -31, -31, -31, -30, -30, -30, -29, -29, -28, -28, -27, -26, -25, -24, -23, 
    -22, -21, -20, -19, -17, -16, -15, -13, -12, -10, -9, -7, -6, -4, -3, -1,
    /* Repeat for second cycle (128-255) to allow easy wrapping */
    0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16, 17, 19, 20, 21, 
    22, 23, 24, 25, 26, 27, 28, 28, 29, 29, 30, 30, 30, 31, 31, 31, 
    32, 31, 31, 31, 30, 30, 30, 29, 29, 28, 28, 27, 26, 25, 24, 23, 
    22, 21, 20, 19, 17, 16, 15, 13, 12, 10, 9, 7, 6, 4, 3, 1, 
    0, -1, -3, -4, -6, -7, -9, -10, -12, -13, -15, -16, -17, -19, -20, -21, 
    -22, -23, -24, -25, -26, -27, -28, -28, -29, -29, -30, -30, -30, -31, -31, -31, 
    -32, -31, -31, -31, -30, -30, -30, -29, -29, -28, -28, -27, -26, -25, -24, -23, 
    -22, -21, -20, -19, -17, -16, -15, -13, -12, -10, -9, -7, -6, -4, -3, -1
};

/* Dithering Characters */
#define CHAR_LIGHT  0xB0 /* 176 */
#define CHAR_MED    0xB1 /* 177 */
#define CHAR_DARK   0xB2 /* 178 */
#define CHAR_SOLID  0xDB /* 219 */

/* * Helper to interpolate between two colors and fill the palette 
 * from index 'start' to 'end'.
 */
void make_gradient(int start, int end, unsigned char col1, unsigned char col2) {
    int i;
    int len = end - start;
    int pos, step;         /* C90: Moved declaration to top */
    unsigned char attr;    /* C90: Moved declaration to top */
    
    for (i = 0; i < len; i++) {
        pos = start + i;
        if (pos >= PALETTE_SIZE) break;

        /* Calculate mix ratio (0 to 4) */
        step = (i * 4) / len; 
        
        attr = (col2 << 4) | col1; /* BG=Col2, FG=Col1 */
        
        switch (step) {
            case 0: /* Mostly Col1 */
                palette_char[pos] = CHAR_SOLID;
                palette_attr[pos] = col1; 
                break;
            case 1: /* Col1 mixing into Col2 (Heavy FG) */
                palette_char[pos] = CHAR_DARK; 
                palette_attr[pos] = attr;
                break;
            case 2: /* Even Mix */
                palette_char[pos] = CHAR_MED;  
                palette_attr[pos] = attr;
                break;
            case 3: /* Mostly Col2 (Light FG) */
                palette_char[pos] = CHAR_LIGHT; 
                palette_attr[pos] = attr;
                break;
            default: /* Solid Col2 */
                palette_char[pos] = CHAR_SOLID;
                palette_attr[pos] = col2;
                break;
        }
    }
}

/* Initialize Color Palette */
void init_tables(void) {
    /* Generate Palette (Rainbow Cycle) 
       Standard CGA Colors:
       1=Blue, 3=Cyan, 2=Green, 10=L.Green, 14=Yellow, 12=L.Red, 4=Red, 5=Magenta, 13=L.Mag, 9=L.Blue
    */
    
    make_gradient(0,  16, 1, 9);   /* Blue -> L.Blue */
    make_gradient(16, 32, 9, 3);   /* L.Blue -> Cyan */
    make_gradient(32, 48, 3, 10);  /* Cyan -> L.Green */
    make_gradient(48, 64, 10, 14); /* L.Green -> Yellow */
    make_gradient(64, 80, 14, 12); /* Yellow -> L.Red */
    make_gradient(80, 96, 12, 13); /* L.Red -> L.Magenta */
    make_gradient(96, 112, 13, 1); /* L.Magenta -> Blue (Loop) */
    make_gradient(112, 128, 1, 1); /* Buffer end */
}

void render_plasma(int t1, int t2, int t3) {
    int r, c, offset;
    int idx;
    /* C90: Variables below moved to top of scope */
    int row_val, col_val, mix_val; 
    
    unsigned char *mem = cga_mem;

    for (r = 0; r < ROWS; r++) {
        /* Use bitwise AND (& 255) to wrap around the table index */
        row_val = sine_table[(r * 4 + t1) & 255]; 
        
        for (c = 0; c < COLS; c++) {
            col_val = sine_table[(c * 2 + t2) & 255];
            mix_val = sine_table[((r + c) * 2 + t3) & 255];
            
            /* Sum components to get a palette index.
               row_val, col_val, mix_val vary -32 to +32.
               Center at 64 to stay within 0-127 range. 
            */
            idx = 64 + row_val + col_val + mix_val;
            
            /* Clamp to palette size */
            if (idx < 0) idx = 0;
            if (idx >= PALETTE_SIZE) idx = PALETTE_SIZE - 1;

            offset = (r * COLS + c) * 2;
            
            mem[offset]     = palette_char[idx];
            mem[offset + 1] = palette_attr[idx];
        }
    }
}

int dos_main(int argc, char *argv[])
{
    union REGS regs;
    int t1 = 0, t2 = 0, t3 = 0;

    cga_mem = MK_FP(0xB800, 0x0000);

    /* Set Text Mode 3 */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs);

    /* Enable High Intensity Backgrounds */
    regs.x.ax = 0x1003;
    regs.h.bl = 0x00; 
    int86(0x10, &regs, &regs);

    init_tables();

    while (!kbhit()) {
        render_plasma(t1, t2, t3);
        
        /* Update animation timers */
        t1 += 3;
        t2 += 5;
        t3 += 7;
        
        delay(50); 
    }

    /* Restore default text mode */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs);

    return 0;
}