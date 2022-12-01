#ifndef _PROCS_H
#define _PROCS_H

#include "types.h"
#include "pcb.h"

#define PROC_DESC_SIZE 0x2000

int32_t sched_next(void);

void init_procs(void);

int8_t get_first_avail_proc_desc(void);

int32_t create_proc_desc(int8_t pid);

PCB* get_pcb(int8_t pid);

void* get_ku_stack(int8_t pid);

uint32_t remove_proc_desc(int8_t pid);

#endif
