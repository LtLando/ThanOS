#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

#define KERNEL_ADDR 0x400000 //4MB - 8MB
#define VIDEO_MEM_ADDR 0xB8000 //4kB within 0-4MB
#define NUM_ENTRIES 1024
#define FOUR_MB_BLOCK NUM_ENTRIES*4 //4096
#define KB_ADDR_OFF 12 // to extract bit range 31:12
#define MB_ADDR_OFF 22 // to extract bit range 31:22
#define PAGE_MULT 4 // page multiplier, multiply by 4 since each page is 4kB
#define SHELL_ADDR 0x800000
#define SECOND_USER_ADDR 0xC00000
#define USER_PDE_IDX 32
#define VID_PDE_IDX 33
#define MB_4_SIZE 0x400000
#define MALLOC_START 209715200

// intialize paging functioning
void init_paging(void);
void init_pd(void);
void init_pt(void);

#endif 
