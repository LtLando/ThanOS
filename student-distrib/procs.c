#include "procs.h"
#include "term_helper.h"
#include "lib.h"
#include "syscalls.h"
#include "x86_desc.h"
#include "paging.h"
#include "syscalls.h"
#include "kmalloc.h"


extern int8_t curr_pid;
extern unsigned char curr_tid;
extern term_t terms[MAX_TERMS];
extern pte_t vidmap_table[NUM_ENTRIES] __attribute__((aligned (FOUR_MB_BLOCK)));
extern char* video_mem;

PCB* proc_descs[NUM_PROCS];


/* int32_t sched_next()
 * Description: 
 * Inputs: None
 * Return Value: -1
 * Side Effects: None
 */
int32_t sched_next(){

    // Increment curr_tid by 1 to switch to next process
    uint8_t next_tid = (curr_tid + 1) % MAX_TERMS;

    asm volatile("movl %%ebp, %0":"=r" (terms[curr_tid].ebp));
    asm volatile("movl %%esp, %0":"=r" (terms[curr_tid].esp));

    // printf("Switching to terminal: %d\n", next_tid);

    // If the next terminal is not running start it up
    if(terms[next_tid].running == 0){

        handle_exec_shell(next_tid);

        // Shouldn't happen
        return 0;

    } else {
    
        //change page
        change_page(terms[next_tid].term_pid+2); //+2 for correct page

        // Set the per-process kernel stack to its correct value
        tss.esp0 = (uint32_t)get_ku_stack(terms[next_tid].term_pid);

        curr_pid = terms[next_tid].term_pid;
 
        curr_tid = next_tid;

        video_mem = (int8_t*)(PIMAGE_VADDR_BASE + (curr_tid * 0x1000));

        asm volatile ("         \n\
            movl %0, %%esp      \n\
            movl %1, %%ebp      \n\
            leave               \n\
            ret                 \n\
            "
            :
            : "r" (terms[next_tid].esp), "r" (terms[next_tid].ebp)
            : "cc"
        );

        // Shouldn't happen
        return 0;
    }
}


void init_procs(){
    uint32_t i;

    for(i = 0; i < NUM_PROCS; i++){
        proc_descs[i] = NULL;
    }
}


int8_t get_first_avail_proc_desc(){
    int8_t i;

    for(i = 0; i < NUM_PROCS; i++){
        if(proc_descs[(uint8_t)i] == NULL){
            if(create_proc_desc(i) == -1){
                return -1;
            }
            return i;
        }
    }

    return -1;
}


int32_t create_proc_desc(int8_t pid){
    if(proc_descs[(uint8_t)pid] != NULL){
        return -1;
    }

    void* mem = kmalloc(PROC_DESC_SIZE);

    if(mem == NULL){
        return -1;
    }

    proc_descs[(uint8_t)pid] = (PCB*)mem;

    init_fdt(pid);

    return 0;
}


uint32_t remove_proc_desc(int8_t pid){
    if(proc_descs[(uint8_t)pid] == NULL){
        return -1;
    }

    kfree((void*)proc_descs[(uint8_t)pid]);

    proc_descs[(uint8_t)pid] = NULL;

    return 0;
}


PCB* get_pcb(int8_t pid){
    return proc_descs[(uint8_t)pid];
}

void* get_ku_stack(int8_t pid){
    void* mem = proc_descs[(uint8_t)pid];

    return (mem + PROC_DESC_SIZE - 0x4);
}
