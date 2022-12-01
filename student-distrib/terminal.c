#include "terminal.h"
#include "lib.h"


extern char KB_BUF[3][BUF_SIZE];
extern volatile uint8_t ENTER_PRESSED[3];
extern unsigned char curr_tid;


/* int32_t terminal_read(char * buf)
 * Description: Copy bytes for the keyboard buffer into the buffer pointed to
 *              by the buf arg. Copy until we hit a '\n' charecter or end
 * Inputs: char* buf = buffer to copy into
 * Return Value: Number of bytes read if success, -1 if fail
 * Side Effects: Modifies the memory at arg buf
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    // Ensure valid buffer pointer
    if(buf == NULL){
        return -1;
    }

    while(ENTER_PRESSED[curr_tid] == 0){}

    char* dest = (char*)(buf);

    int32_t idx;
    int32_t clear_idx;

    // Loop from 0 to n copying chars
    for(idx = 0; idx < nbytes; idx++){
        dest[idx] = KB_BUF[curr_tid][idx];

        // if we hit a newline stop
        if(KB_BUF[curr_tid][idx] == '\n'){
            break;
        }
    }

    for(clear_idx = 0; clear_idx < BUF_SIZE; clear_idx++){
        KB_BUF[curr_tid][clear_idx] = 0;
    }

    ENTER_PRESSED[curr_tid] = 0;

    // Num copies = index + 1 due to zero indexing
    return idx + 1;
}


/* int32_t terminal_write(char * buf)
 * Description: Copy bytes from buf into the terminal
 * Inputs: char* buf = buffer write from
 * Return Value: Number of bytes written if success, -1 if fail
 * Side Effects: Modifies video memory
 */
int32_t terminal_write(int32_t fd, void* buf, int32_t nbytes){
    // Ensure valid buffer pointer
    if(buf == NULL){
        return -1;
    }

    int32_t idx;
    char* src = (char*)(buf);

    // Loop from 0 to n copying chars
    for(idx = 0; idx < nbytes; idx++){
        if(src[idx] != 0)
            putc(src[idx], curr_tid);
    }
    
    // Num copies = index of loop
    return idx;
}


/* int32_t terminal_open()
 * Description: Open function for the terminal
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t terminal_open(const int8_t* filename){
    return 0;
}


/* int32_t terminal_close()
 * Description: Close function for the terminal
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t terminal_close(int32_t fd) {
    return -1;
}
