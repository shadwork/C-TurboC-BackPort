#include "matrix.h"

/* Screen dimensions for CGA Mode 3 */
#define COLS 80
#define ROWS 25
#define CGA_BUFFER_SIZE (COLS * ROWS * 2) /* 4000 bytes */

/* CGA Memory Pointer (0xB800:0000) */
unsigned char *cga_mem;

/* Attributes for the rain effect */
#define BRIGHT_GREEN 0x0A
#define DARK_GREEN   0x02
#define BLACK        0x00

/* Defines the state of each column (rain drop) */
typedef struct {
    int tail_len;
    int current_y;
    int drop_speed;
} RainDrop;

/* Array to hold the state of all 80 columns */
RainDrop drops[COLS];

/* Function to clear the screen using DMA */
void clear_screen() {
    int i;
    /* 80 columns * 25 rows * 2 bytes/cell = 4000 bytes */
    for (i = 0; i < COLS * ROWS * 2; i += 2) {
        cga_mem[i] = ' ';         /* Character space */
        cga_mem[i + 1] = BLACK;   /* Attribute black */
    }
}

/* Function to initialize the drops array */
void init_drops() {
    int i;
    for (i = 0; i < COLS; i++) {
        drops[i].tail_len = rand() % 10 + 5; /* Tail length 5 to 14 */
        drops[i].current_y = rand() % ROWS;  /* Random start Y */
        drops[i].drop_speed = rand() % 4 + 1;/* Speed 1 to 4 */
    }
}

/* Function to render one frame of the rain */
void render_rain() {
    int col, row, offset, fade_level, relative_pos;

    for (col = 0; col < COLS; col++) {
        /* Update the position of the rain drop's head */
        drops[col].current_y = (drops[col].current_y + drops[col].drop_speed) % (ROWS + drops[col].tail_len);

        /* Render the rain column */
        for (row = 0; row < ROWS; row++) {
            offset = (row * COLS + col) * 2; 

            /* Calculate the position relative to the head */
            relative_pos = drops[col].current_y - row;

            if (relative_pos == 0) {
                /* --- 1. The Head --- */
                cga_mem[offset] = rand() % 95 + 32; 
                cga_mem[offset + 1] = BRIGHT_GREEN;
            } 
            else if (relative_pos > 0 && relative_pos <= drops[col].tail_len) {
                /* --- 2. The Tail --- */
                fade_level = relative_pos; 
                
                if (fade_level <= 3) {
                    cga_mem[offset] = rand() % 95 + 32;
                    cga_mem[offset + 1] = DARK_GREEN;
                } else if (fade_level == drops[col].tail_len) {
                    cga_mem[offset] = ' ';
                    cga_mem[offset + 1] = BLACK;
                } else {
                    cga_mem[offset + 1] = DARK_GREEN;
                }
            } 
            else {
                /* --- 3. EVERYTHING ELSE (Fix) --- */
                /* If it's not head or tail, force it to black.
                This automatically cleans up the bottom row when the 
                drop moves off-screen. */
                cga_mem[offset] = ' ';
                cga_mem[offset + 1] = BLACK;
            }
        }

    }
}

int dos_main(int argc, char *argv[])
{
union REGS regs;

    /* 1. Set CGA Memory Pointer */
    cga_mem = MK_FP(0xB800, 0x0000);

    /* 2. Set Color Text Mode */
    regs.h.ah = 0;
    regs.h.al = 3;
    int86(0x10, &regs, &regs);

    /* 3. Initialize Randomizer and Rain Drops */
    srand(time(NULL));
    init_drops();
    clear_screen();

    /* 4. Main Rendering Loop */
    while (!kbhit()) { /* Loop until a key is pressed */
        render_rain();
        delay(50); /* Adjust this for speed (e.g., 50 milliseconds) */
    }
    return 0;
}