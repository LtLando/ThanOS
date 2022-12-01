#ifndef _PIT_H
#define _PIT_H

#include "types.h"

#define PIT_CMD 0x43
#define PIT_DATA 0x40
#define PIT_CHANNEL 0
#define PIT_ACCESS_MODE 3
#define PIT_OPERATING_MODE 3
#define PIT_BINARY_MODE 1
#define PIT_INIT_WORD ((PIT_CHANNEL << 6) | (PIT_ACCESS_MODE << 4) | (PIT_OPERATING_MODE << 1) | PIT_BINARY_MODE)
#define PIT_INTERVAL 11932

void init_pit(void);

void handle_pit(void);

void sleep(uint32_t count);

#endif
