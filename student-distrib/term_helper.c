#include "term_helper.h"
#include "lib.h"
#include "pcb.h"
#include "syscalls.h"
#include "x86_desc.h"
#include "paging.h"
#include "syscalls.h"

extern int8_t curr_pid;
term_t terms[MAX_TERMS];
unsigned char curr_tid = 0x00; //0, 1, or 2, 6MSB not used
unsigned char shown_tid = 0x00;
extern pte_t vidmap_table[NUM_ENTRIES] __attribute__((aligned (FOUR_MB_BLOCK)));

/* int32_t term_open()
 * Description: Open new terminal
 * Inputs: tid: terminal ID to be opened
 * Return Value: 0 on sucess, -1 on failure
 * Side Effects: None
 */
int32_t term_open(uint8_t tid) {
    if(tid > MAX_TID) return -1;

    terms[tid].running = 1;

    uint32_t pte_idx = ((PIMAGE_VADDR_BASE + (tid * FOUR_KB_BLOCK)) >> KB_ADDR_OFF) & 0x3FF; //x3FF to get 10 LSB
    vidmap_table[pte_idx].addr_31_12 = VIDEO_MEM_ADDR >> KB_ADDR_OFF;
    asm volatile("          \n\
        movl %%cr3, %%eax   \n\
        movl %%eax, %%cr3   \n\
        "
        :
        :
        : "eax", "cc"
    );

    sti();
    handle_exec_shell(tid);

    return 0;
}

/* int32_t term_close()
 * Description: Close a certain terminal
 * Inputs: tid: terminal ID to be closed
 * Return Value: 0 on sucess, -1 on failure
 * Side Effects: None
 */
int32_t term_close(uint8_t tid) {
    if (tid > MAX_TID) return -1;
    terms[tid].running = 0;
    return 0;
}

/* int32_t term_switch()
 * Description: Switch to a different terminal by loading in the new video memory location and saving/restoring screen data
 * Inputs: new_shown_tid: terminal ID of destination terminal 
 * Return Value: 0 on success, -1 on failure
 * Side Effects: None
 */
int32_t term_switch(uint8_t new_shown_tid) {
    // IF tid invalid or if we are already on the one requested to switch to ret -1
    // if (tid > MAX_TID || curr_tid == tid) return -1;
    if (new_shown_tid > MAX_TID) return -1;

    uint32_t pte_idx = ((PIMAGE_VADDR_BASE + (shown_tid * FOUR_KB_BLOCK)) >> KB_ADDR_OFF) & 0x3FF; //x3FF to get 10 LSB
    vidmap_table[pte_idx].addr_31_12 = ((PIMAGE_VADDR_BASE + (shown_tid * FOUR_KB_BLOCK)) >> KB_ADDR_OFF);
    asm volatile("          \n\
        movl %%cr3, %%eax   \n\
        movl %%eax, %%cr3   \n\
        "
        :
        :
        : "eax", "cc"
    );

    copy_from_screen((int8_t*)(PIMAGE_VADDR_BASE + (shown_tid * FOUR_KB_BLOCK)));

    copy_to_screen((int8_t*)(PIMAGE_VADDR_BASE + (new_shown_tid * FOUR_KB_BLOCK)));

    pte_idx = ((PIMAGE_VADDR_BASE + (new_shown_tid * FOUR_KB_BLOCK)) >> KB_ADDR_OFF) & 0x3FF; //x3FF to get 10 LSB
    vidmap_table[pte_idx].addr_31_12 = VIDEO_MEM_ADDR >> KB_ADDR_OFF;
    asm volatile("          \n\
        movl %%cr3, %%eax   \n\
        movl %%eax, %%cr3   \n\
        "
        :
        :
        : "eax", "cc"
    );

    shown_tid = new_shown_tid;

    update_cursor(terms[shown_tid].screen_x, terms[shown_tid].screen_y);

    return 0;
}

/* int32_t find_open_term()
 * Description: Find running terminal by searching in increasing order
 * Inputs: None
 * Return Value: Index of open terminal, -1 on failure
 * Side Effects: None
 */
int32_t find_open_term(){
    int32_t idx;

    for(idx = 0; idx < MAX_TERMS; idx++){
        if(terms[idx].running != 0){
            return idx;
        }
    }

    return -1;
}

/* void init_term_video()
 * Description: Clear the terminal by filling it with spaces
 * Inputs: None
 * Return Value: None
 * Side Effects: None
 */
void init_term_video(){

    uint32_t i;

    for(i = 0; i < MAX_TERMS; i++){
        clear_specific(i);
    }
}
