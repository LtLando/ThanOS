#ifndef _PCB_H
#define _PCB_H

#include "filesys.h"


#define FDT_SIZE 8
#define PCB_START_LOC 0x800000
#define PCB_SIZE 0x2000
#define NUM_PROCS 30
#define MAX_CHAR_ARG 1024


typedef struct PCB {
    uint8_t active;
    // uint32_t old_stack;
    uint32_t old_base;
    int8_t parent_pid;
    int8_t pid;
    file_desc_table_entry fdt[FDT_SIZE];
    int8_t args[1024]; // used for get_args to store arguments
    int8_t tid; // terminal id
} PCB;


void init_fdt(int8_t pid);
// PCB* get_pcb(int8_t pid);

#endif
