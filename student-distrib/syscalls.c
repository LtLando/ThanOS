#include "syscalls.h"
#include "lib.h"
#include "terminal.h"
#include "filesys.h"
#include "pcb.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"
#include "term_helper.h"
#include "procs.h"

extern pde_t page_dir[NUM_ENTRIES] __attribute__ ((aligned (FOUR_MB_BLOCK)));
extern pte_t vidmap_table[NUM_ENTRIES] __attribute__((aligned (FOUR_MB_BLOCK)));
extern int8_t curr_pid;
extern uint8_t curr_tid;
extern term_t terms[3];
extern char* video_mem;
extern uint8_t sched_en;

/* int32_t change_page
 * Description: Helper function to switch page when changing process
 * Inputs: uint32_t arg
 * Return Value: 0
 * Side Effects: Sets pdbr soter as to change page for sys call functions
 */
int32_t change_page(uint32_t arg){
    page_dir[USER_PDE_IDX].pde_mb.addr_31_22 = arg;

    // Flush TLB
    asm volatile("          \n\
        movl %%cr3, %%eax   \n\
	    movl %%eax, %%cr3   \n\
        "
        :
        :
        : "eax", "cc"
    );

    return 0;
}

/* int32_t handle_syscall1
 * Description: Halt systcall
 * Inputs: arg1 is uint8_t status
 * Return Value: 0
 * Side Effects: Halts current process and returns control to parent shell
 */
int32_t sys_halt(uint32_t arg1, uint32_t arg2, uint32_t arg3){

    cli();

    // Get the current processes PCB
    PCB* pcb = get_pcb(curr_pid);
    int8_t old_pid = curr_pid;

    // Loop through and set all files as unused (except stdin, stdout)
    uint32_t i;
    for(i = 2; i < 8; i++){ //2 to skip stdin, stdout, 8 total files possible
        pcb->fdt[i].flags = 0;
        pcb->fdt[i].inode = 0;
        pcb->fdt[i].op_ptr = NULL;
        pcb->fdt[i].file_pos = 0;
    }
    
    pcb->active = 0;

    // If process is overarching shell, don't halt
    if(pcb->parent_pid == PER_TERM_ROOT_PROC_ID) {
        // term_close(curr_tid);
        remove_proc_desc(old_pid);
        handle_exec_shell(curr_tid);
    }

    curr_pid = pcb->parent_pid;
    terms[curr_tid].term_pid = curr_pid;

    remove_proc_desc(old_pid);
    
    // Set the process to be not active
    change_page(curr_pid+2); // +2 for new page

    //reset TSS
    tss.esp0 = (uint32_t)get_ku_stack(curr_pid);

    // Place the saved base pointer from the execute call back into ebp
    // therefore when we do a leave the stack from the first execute call will be torn
    // down (when leave does: movl %ebp, %esp), then the ret will take us back the the assm linkage
    // and the return code of this process is placed into eax (return value register) thus, to
    // the process that called execute it appears that the return value came from that function
    // when in reality it came from the child function
    asm volatile ("         \n\
        movl %%edx, %%ebp   \n\
        movl %%ebx, %%eax   \n\
        leave               \n\
        ret                 \n\
        "
        :
        : "d" (pcb->old_base), "b" (arg1)
        : "eax", "ebp"
    );

    // Should never happen
    printf("TEST");
    return -1;
}

/* int32_t handle_exec_shell
 * Description: Initializes shell executable
 * Inputs: None
 * Return Value: None (shell always open)
 * Side Effects: sets up initial shell for execute
 */
int32_t handle_exec_shell(uint8_t tid){
    //check for executable
    char checkBuf[4]; //4 to only check bytes 1-3
    if (check_exec("shell", checkBuf) != 4 || strncmp("ELF", checkBuf+1, 3) != 0) return -1; //4 and then incremented checkBuf comparing with size 3 to only check bytes 1-3

    //load file into memory
    exec_open("shell");

    //set current pid
    int8_t child_pid = get_first_avail_proc_desc();
    PCB* pcb_loc = get_pcb(child_pid); //add 2 to index into right pcb memory location
    pcb_loc->active = 1; // 1 for now active
    pcb_loc->pid = child_pid;
    pcb_loc->parent_pid = PER_TERM_ROOT_PROC_ID;
    pcb_loc->tid = tid;

    terms[tid].running = 1;
    terms[tid].term_pid = child_pid;
    curr_pid = child_pid;
    curr_tid = tid;

    video_mem = (int8_t*)(PIMAGE_VADDR_BASE + (curr_tid * 0x1000));

    clear();
    set_screen_pos(0, 0);

    change_page(child_pid+2);
    exec_read((int8_t *) (PIMAGE_VADDR), exec_get_filesize());

    //push IRET context to kernel stack
    uint32_t entry_point = exec_get_entry("shell");

    //prepare for context switch
    tss.ss0 = KERNEL_DS;
    tss.esp0 = (uint32_t)get_ku_stack(child_pid);

    sched_en = 1;
    sti();

    asm volatile ("                     \n\
            movw %%bx, %%ds             \n\
	        movw %%bx, %%es             \n\
	        movw %%bx, %%fs             \n\
	        movw %%bx, %%gs             \n\
                                        \n\
            pushl %%ebx                 \n\
            pushl %%edx                 \n\
            pushfl                      \n\
            pushl %%ecx                 \n\
            pushl %%eax                 \n\
            iret                        \n\
        "
        :
        : "a"(entry_point), "b"(USER_DS), "c"(USER_CS), "d"(PIMAGE_VADDR_BASE - PCB_OFF)
        : "cc"
    );

    // Should never happen
    return -1;
}

/* int32_t handle_syscall2
 * Description: Initializes executable file
 * Inputs: arg1 is const uint8_t command
 * Return Value: 0
 * Side Effects: Executes based of command, calls our handler as well
 */
int32_t sys_execute(uint32_t arg1, uint32_t arg2, uint32_t arg3){

    int k;
    int c = 0;
    unsigned flag = 0;
    uint8_t* temp = (uint8_t*)arg1;
    uint8_t cmd[128]; // 128 is total number of characters we can have in terminal
    for(k = 0; k < 128; k++){ // 128 is make space of buffer
        if(flag){ // if we are copying exec
            if(temp[k] == ' '){ // if space we end
                cmd[c] = 0;
                break;
            } else {
                cmd[c] = temp[k];
                c++;
            }
        } else {
            if(temp[k] == ' '){ // still not copying section
                continue;
            } else {
                flag = 1;
                cmd[c] = temp[k];
                c++;
            }
        }
    }

    //check for executable
    char checkBuf[4]; //4 to only check bytes 1-3
    if (check_exec((const int8_t *) cmd, checkBuf) != 4 || strncmp("ELF", checkBuf+1, 3) != 0 || cmd[0] == NULL) return -1; //4 and then incremented checkBuf comparing with size 3 to only check bytes 1-3
    
    int32_t fd = file_open((const int8_t*) cmd);
    if(fd == -1) return -1; //-1 is parent pid

    // Setup child processes PCB
    int8_t child_pid = get_first_avail_proc_desc();
    if(child_pid == -1) return -1;
    PCB* child_pcb = get_pcb(child_pid);

    child_pcb->active = 1;
    child_pcb->pid = child_pid;
    child_pcb->parent_pid = curr_pid;
    child_pcb->tid = get_pcb(curr_pid)->tid;

    int i;
    flag = 0;
    c = 0;
    for(i = k; i < 128; i++){ // copying args from rest of the buf, 128 is total buffer size
        if(flag){ // can just copy everything over after the first non_space
            child_pcb->args[c] = temp[i];
            c++;
        } else {
            if(temp[i] == ' '){
                continue;
            } else {
                flag = 1;
                child_pcb->args[c] = temp[i]; // copy arg to memory
                c++;
            }
        }
    }

    change_page(child_pid+2); //+4 to index to new page

    //load file into memory
    uint32_t file_len = get_inode(get_pcb(curr_pid)->fdt[fd].inode)->length;
    if (file_read(fd, (int8_t *) (PIMAGE_VADDR), file_len) != file_len) return -1;
    file_close(fd);

    //push IRET context to kernel stack
    // 0x08048000
    uint32_t entry_point;
    uint32_t byte27 = *(((int8_t *) (PIMAGE_VADDR)) + 27)<<BYTE27_SHIFT; //byte 27
    uint32_t byte26 = *(((int8_t *) (PIMAGE_VADDR)) + 26)<<BYTE26_SHIFT & BYTE26_MASK; //byte 26
    uint16_t byte25 = *(((int8_t *) (PIMAGE_VADDR)) + 25)<<BYTE25_SHIFT; //byte 25
    uint8_t byte24 = *(((int8_t *) (PIMAGE_VADDR)) + 24); //byte 24
    entry_point = byte27 | byte26 | byte25 | byte24;
    //prepare for context switch

    curr_pid = child_pid;   
    terms[curr_tid].term_pid = curr_pid;

    tss.ss0 = KERNEL_DS;
    // Set the per-process kernel stack to its correct value
    tss.esp0 = (uint32_t)get_ku_stack(curr_pid); //add 1 to index into right pcb memory location

    // Place the current base pointer into the new processes old_base elem
    // in order to be able to return to here on call to halt
    asm volatile("movl %%ebp, %0":"=r" (child_pcb->old_base));

    asm volatile ("                     \n\
            movw %%bx, %%ds             \n\
	        movw %%bx, %%es             \n\
	        movw %%bx, %%fs             \n\
	        movw %%bx, %%gs             \n\
                                        \n\
            pushl %%ebx                 \n\
            pushl %%edx                 \n\
            pushfl                      \n\
            pushl %%ecx                 \n\
            pushl %%eax                 \n\
        "
        :
        : "a"(entry_point), "b"(USER_DS), "c"(USER_CS), "d"(PIMAGE_VADDR_BASE - PCB_OFF)
        : "cc"
    );

    asm volatile ("     \n\
        iret            \n\
    ");

    // Should never happen
    return -1;
}


/* int32_t sys_read
 * Description: Read syscall
 * Inputs: int32 t fd, void* buf, int32 t nbytes
 * Return Value: number of bytes read
 * Side Effects: Reads in the file descriptor and performs read dependent on filetype
 */
int32_t sys_read(uint32_t arg1, uint32_t arg2, uint32_t arg3){
    if(arg1 > MAX_FD || arg1 < MIN_FD) return -1;
    
    void* buf = (void*)(arg2);
    if(buf == NULL) return -1;

    int32_t fd = (int32_t)(arg1);
    int32_t nbytes = (int32_t)(arg3);

    if(nbytes < 0) return -1; //can't read less than 0 bytes
    if(get_pcb(curr_pid)->fdt[fd].flags == 0) return -1; // Only allow operation if file is active

    if(get_pcb(curr_pid)->fdt[fd].op_ptr->read == NULL) return -1;

    return get_pcb(curr_pid)->fdt[fd].op_ptr->read(fd, buf, nbytes);
}


/* int32_t sys_write
 * Description: Write syscall
 * Inputs: int32 t fd, void* buf, int32 t nbytes
 * Return Value: number of bytes written
 * Side Effects: performs one of the 4 different file_op writes based on filetype from fd
 */
int32_t sys_write(uint32_t arg1, uint32_t arg2, uint32_t arg3){

    int32_t fd = (int32_t)(arg1);
    int32_t nbytes = (int32_t)(arg3);
    if((fd > MAX_FD) || ((fd < MIN_FD) && (fd != -2))) return -1;
    
    void* buf = (void*)(arg2);
    if(buf == NULL) return -1;

    if(nbytes < 0) return -1; //can't write less than 0 bytes
    //if(get_pcb(curr_pid)->fdt[arg1].flags == 0) return -1; // Only allow operation if file is active

    if (fd == -2) return file_write(fd, buf, NULL); //new file

    if(get_pcb(curr_pid)->fdt[fd].flags == 0) return -1; // Only allow operation if file is active

    if(get_pcb(curr_pid)->fdt[fd].op_ptr->write == NULL) return -1;

    return get_pcb(curr_pid)->fdt[fd].op_ptr->write(fd, buf, nbytes);
}


/* int32_t sys_open
 * Description: Open syscall
 * Inputs: arg1 is const int8_t filename
 * Return Value: file descriptor generated from file_open
 * Side Effects: calls file_open and returns a fd
 */
int32_t sys_open(uint32_t arg1, uint32_t arg2, uint32_t arg3){
    if ((const int8_t*) arg1 == NULL) return -1; // fails for invalid filename
    return file_open((const int8_t*) arg1);
}


/* int32_t sys_close
 * Description: close systcall
 * Inputs: arg1 is int32 t fd
 * Return Value: 0 if valid fd to close or -1 if invalid
 * Side Effects: calls file_close from filesys.h to remove a file from pcb's fdt
 */
int32_t sys_close(uint32_t arg1, uint32_t arg2, uint32_t arg3){
    return file_close((int32_t) arg1);
}

/* int32_t get_args
 * Description: get_args systcall
 * Inputs: uint8 t* buf, int32 t nbytes
 * Return Value: 0 if valid arg to close or -1 if invalid
 * Side Effects: 
 */
int32_t get_args(uint32_t arg1, uint32_t arg2, uint32_t arg3){
    int8_t* buf = (int8_t*)arg1;
    PCB* cur_pcb = get_pcb(curr_pid);
    
    if(buf == NULL || cur_pcb->args[0] == NULL){
        return -1;
    }
    strcpy(buf, cur_pcb->args);
    int i;
    for(i = 0; i < 128; i++){ // length of the args array is 128 so fill with null
        cur_pcb->args[i] = 0;
    }
    return 0;
}

/* int32_t sys_vidmap
 * Description: vidmap systcall
 * Inputs: uint8 t** screen_start
 * Return Value: 0 if valid arg to or -1
 * Side Effects: Sets up paging structure to display to video memory
 */
int32_t sys_vidmap(uint32_t arg1, uint32_t arg2, uint32_t arg3){
    uint8_t** screen_start = (uint8_t**)arg1;
    if (screen_start == NULL || arg1 < SHELL_ADDR || arg1 > PIMAGE_VADDR_BASE){ // checks if screen start out of range or greater than the shell + 4MB range
        return -1;
    }

    *screen_start = (uint8_t*)(PIMAGE_VADDR_BASE + (curr_tid * 0x1000)); // Set screen start to page for terminal specific video memory
    return 0;
}
