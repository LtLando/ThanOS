#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

#define BUF_SIZE 128

// Read the keyboard buffer into pointed to buffer
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

// Open function for terminal
int32_t terminal_open(const int8_t* filename);

// Write buff to the display
int32_t terminal_write(int32_t fd, void* buf, int32_t nbytes);

// Close function for terminal
int32_t terminal_close(int32_t fd);

#endif
