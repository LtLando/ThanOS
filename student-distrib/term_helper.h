#ifndef _TERM_HELPER_H
#define _TERM_HELPER_H

#define MAX_TID 2
#define MIN_TID 0
#define MAX_TERMS 3

#include "types.h"

typedef struct term {
    uint8_t running;
    uint8_t term_pid;
    int8_t vid_copy[4000];
    int32_t ebp;
    int32_t esp;
    int32_t screen_x;
    int32_t screen_y;
} term_t;

int32_t term_open(uint8_t tid);
int32_t term_close(uint8_t tid);
int32_t term_switch(uint8_t tid);
int32_t find_open_term(void);
void init_term_video(void);

#endif
