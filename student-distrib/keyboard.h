#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define KB_PIC_PORT 0x1

#define KB_IN_PORT 0x60

#define PRESS_LIMIT 0x80 // where the pressed scancodes end
#define BUF_LIMIT 127
#define READ_LIMIT 0x3A

// modifier scancodes
#define SHIFT_R 0x36
#define SHIFT_L 0x2A
#define SHIFT_L_RELEASED 0xAA
#define SHIFT_R_RELEASED 0xB6

#define CAPS_LOCK 0x3A
#define CTRL_PRESSED 0x1D
#define CTRL_RELEASED 0x9D
#define CAPS_RELEASED 0xBA
#define UP_PRESSED 0x48
#define DOWN_PRESSED 0x50

#define L_ALT_PRESSED 0x38
#define L_ALT_RELEASED 0xB8

#define LEFT_ARROW 0x4B
#define RIGHT_ARROW 0x4D

#define TERM_1 0x3B
#define TERM_2 0x3C
#define TERM_3 0x3D

#define BACKSPACE_PRESSED 0x0E // scancode for backspace
#define NUM_PREVS 100 // Number of kept commands

#define L_CODE 0x26 // scancode for l

#include "types.h"

// checking if scancode is alphanumeric
int alphanumeric(uint8_t scancode);

// main handler to fill buffer
void default_kb_handler(void);

// func to initialize the keyboard and buffer
void keyboard_init(void);

void cmd_history_prev(void);

#endif
