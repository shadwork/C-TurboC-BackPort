#ifndef BIOS_DATA_AREA_H
#define BIOS_DATA_AREA_H

#include <stdint.h>

/*
 * IBM PC BIOS Data Area (BDA)
 * Located at Segment 0x0040 (Physical Address 0x00400)
 */

// --- BDA Offset Definitions (Relative to 0x0000, typically Segment 0x40) ---
#define BDA_COM1_PORT       0x400
#define BDA_COM2_PORT       0x402
#define BDA_COM3_PORT       0x404
#define BDA_COM4_PORT       0x406
#define BDA_LPT1_PORT       0x408
#define BDA_LPT2_PORT       0x40A
#define BDA_LPT3_PORT       0x40C
#define BDA_LPT4_PORT       0x40E

#define BDA_EQUIPMENT_LIST  0x410
#define BDA_MEMORY_SIZE_KB  0x413
#define BDA_KBD_STATUS_1    0x417
#define BDA_KBD_STATUS_2    0x418
#define BDA_KBD_BUFFER_HEAD 0x41A
#define BDA_KBD_BUFFER_TAIL 0x41C
#define BDA_KBD_BUFFER      0x41E

#define BDA_FDD_MOTOR_STATUS 0x43F
#define BDA_FDD_STATUS_RET   0x441

#define BDA_VIDEO_MODE      0x449
#define BDA_VIDEO_COLS      0x44A
#define BDA_VIDEO_PAGE_SIZE 0x44C
#define BDA_VIDEO_PAGE_OFF  0x44E
#define BDA_CURSOR_POS      0x450
#define BDA_CURSOR_TYPE     0x460
#define BDA_ACTIVE_PAGE     0x462
#define BDA_CRT_CONTROLLER  0x463
#define BDA_MODE_SELECT_REG 0x465
#define BDA_PALETTE_ID      0x466

#define BDA_TIMER_TICKS     0x46C
#define BDA_TIMER_OVERFLOW  0x470
#define BDA_BREAK_FLAG      0x471
#define BDA_RESET_FLAG      0x472

/*
 * Structure mapping the hardware variables maintained by the BIOS.
 * The __attribute__((packed)) is essential to prevent compiler padding.
 */
#pragma pack(push, 1)

typedef struct {
    // --- Serial & Parallel Ports (0x400 - 0x40F) ---
    uint16_t com_ports[4];      // 0x00: I/O addresses for COM1-COM4
    uint16_t lpt_ports[4];      // 0x08: I/O addresses for LPT1-LPT4

    // --- Equipment & Memory (0x410 - 0x416) ---
    uint16_t equipment_list;    // 0x10: Equipment word (bits indicate HW installed)
    uint8_t  reserved_1;        // 0x12: Mfg. test flag
    uint16_t memory_size_kb;    // 0x13: Base memory size in KB (0-640)
    uint8_t  reserved_2;        // 0x15: Mfg. error codes
    uint8_t  reserved_3;        // 0x16: Unused

    // --- Keyboard Status (0x417 - 0x43D) ---
    /* * 0x17: Keyboard Shift Status 1 (BDA_KBD_STATUS_1)
     * Bit 7: Insert | 6: Caps | 5: Num | 4: Scroll | 3: Alt | 2: Ctrl | 1: L-Shift | 0: R-Shift
     */
    uint8_t  kbd_status_1;      

    /* * 0x18: Keyboard Shift Status 2 (BDA_KBD_STATUS_2)
     * Bit 7: Ins Pressed | 6: Caps Pressed | 5: Num Pressed | 4: Scroll Pressed | 3: Pause | 2: SysRq ...
     */
    uint8_t  kbd_status_2;      
    uint8_t  alt_keypad_entry;  // 0x19: Storage for Alt+Keypad entry
    uint16_t kbd_buf_head;      // 0x1A: Pointer to head of circular buffer
    uint16_t kbd_buf_tail;      // 0x1C: Pointer to tail of circular buffer
    uint8_t  kbd_buffer[32];    // 0x1E: 16-char circular keystroke buffer

    // --- Diskette Data (0x43E - 0x448) ---
    uint8_t  fdd_calibration;   // 0x3E: Drive recalibration status
    uint8_t  fdd_motor_status;  // 0x3F: Motor status
    uint8_t  fdd_motor_timeout; // 0x40: Motor shutoff counter
    uint8_t  fdd_status_ret;    // 0x41: Status of last operation
    uint8_t  fdd_controller[7]; // 0x42: Controller status bytes

    // --- Video Display Data (0x449 - 0x466) ---
    uint8_t  video_mode;        // 0x49: Current video mode
    uint16_t video_cols;        // 0x4A: Number of columns on screen
    uint16_t video_page_size;   // 0x4C: Size of current video page in bytes
    uint16_t video_page_off;    // 0x4E: Offset of current page in segment
    uint16_t cursor_pos[8];     // 0x50: Cursor position (col, row) for 8 pages
    uint16_t cursor_type;       // 0x60: Cursor start/end scan lines
    uint8_t  active_page;       // 0x62: Currently active page number
    uint16_t crt_controller;    // 0x63: I/O port of CRT controller (3B4h or 3D4h)
    uint8_t  mode_select_reg;   // 0x65: Current setting of 3x8 register
    uint8_t  palette_id;        // 0x66: Current palette

    // --- System Timer (0x467 - 0x46C) ---
    uint16_t reserved_4;        // 0x67: Cassette/POST data
    uint16_t reserved_5;        // 0x69: Unused
    uint32_t timer_ticks;       // 0x6C: Daily timer ticks (18.2 Hz)
    uint8_t  timer_overflow;    // 0x70: 24-hour overflow flag

    // --- Misc (0x471 - 0x472) ---
    uint8_t  break_flag;        // 0x71: Bit 7=1 if Ctrl-Break pressed
    uint16_t reset_flag;        // 0x72: 0x1234 = Warm boot (skip mem check)

    // ... Additional fields extend to offset 0x100 ...

} bios_data_area_t;

#pragma pack(pop)

/* * Helper macro to access the BDA at physical address 0x400. */
#define BDA ((volatile bios_data_area_t *)0x00000400)

#endif // BIOS_DATA_AREA_H