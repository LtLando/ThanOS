#include "pcb.h"
#include "terminal.h"
#include "rtc.h"
#include "filesys.h"
#include "procs.h"


// typedef int32_t (*open_handler)(const uint8_t* filename);
// typedef int32_t (*close_handler)(int32_t fd);
// typedef int32_t (*read_handler)(int32_t fd, void* buf, int32_t nbytes);
// typedef int32_t (*write_handler)(int32_t fd, void* buf, int32_t nbytes);


struct file_op_table stdout_ops = {terminal_open, terminal_close, (read_handler)NULL, terminal_write};
struct file_op_table stdin_ops = {terminal_open, terminal_close, terminal_read, (write_handler)NULL};
struct file_op_table rtc_ops = {rtc_open, rtc_close, rtc_read, rtc_write};
struct file_op_table dir_ops = {dir_open, dir_close, dir_read, (write_handler)NULL};
struct file_op_table reg_ops = {file_open, file_close, file_read, file_write};

int8_t curr_pid;



// PCB* get_pcb(int8_t pid){
//     return (PCB*)(PCB_START_LOC - ((pid+1)*PCB_SIZE));
// }


void init_fdt(int8_t pid){   
    PCB* pcb_loc = get_pcb(pid);
    // Clear all PCBs for processes 1-6
    uint32_t idx;
    for(idx = 0; idx < FDT_SIZE; idx++){
        if(idx == 0){
            pcb_loc->fdt[idx].flags = 1; // 1 For active file
            pcb_loc->fdt[idx].inode = 0;
            pcb_loc->fdt[idx].op_ptr = &stdin_ops;
            pcb_loc->fdt[idx].file_pos = 0;
        } else if(idx == 1){
            pcb_loc->fdt[idx].flags = 1; // 1 For active file
            pcb_loc->fdt[idx].inode = 0;
            pcb_loc->fdt[idx].op_ptr = &stdout_ops;
            pcb_loc->fdt[idx].file_pos = 0;
        } else {
            pcb_loc->fdt[idx].flags = 0;
            pcb_loc->fdt[idx].inode = 0;
            pcb_loc->fdt[idx].op_ptr = NULL;
            pcb_loc->fdt[idx].file_pos = 0;
        }
    }
}
