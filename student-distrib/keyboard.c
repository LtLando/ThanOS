#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "term_helper.h"


char KB_BUF[3][128]; // buffer used by terminal to display values
char prev_buf[NUM_PREVS][3][128];
uint32_t prev_len[NUM_PREVS][3];
volatile uint8_t ENTER_PRESSED[3] = {0,0,0}; // used to check whether end of line and change cursor loc
extern unsigned char shown_tid;
int32_t curr_hist_pnt[3] = {-1, -1, -1};
int32_t hist_max[3] = {-1, -1, -1};
int32_t cursor_pos[3] = {0, 0, 0};

// scancode map for no modifiers
char reg_kb_map [128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w',
    'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',
    ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h',
    'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

// scancode map for caps lock
char caps_kb_map [128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '0', '-', '=', '\b', '\t', 'Q', 'W',
    'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[',
    ']', '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L', ';', '\'', '`',  0, '\\', 'Z',
    'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

// scancode map for shift pressed
char shift_kb_map[128]={
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', 
    '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 
    'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 
    '"', '~',   0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 
    'M', '<', '>', '?',   0, '*', 0, ' ', 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


// flags for checking modifiers
static char caps_flag = 0;
static char ctrl_flag = 0;
static char shift_flag = 0;
static char alt_flag = 0;

// var to track how many chars in buffer
int char_ct[3] = {0};


/* void keyboard_init()
 * Description: Initializes the PS2 Keyboard device
 * Inputs: None
 * Return Value: None
 * Side Effects: Bytes written to PIC
 */
void keyboard_init() {
    int i, j, k;
    for(i = 0; i < 128; i++){ // sets the 128 size buffer to all be null chars to start
        for(j = 0; j < 3; j++){
            KB_BUF[j][i] = 0;
        }
    }

    for(i = 0; i < 128; i++){ // sets the 128 size buffer to all be null chars to start
        for(j = 0; j < 3; j++){
            for(k = 0; k < NUM_PREVS; k++){
                prev_buf[k][j][i] = 0;
            }
        }
    }

    enable_irq(KB_PIC_PORT);
}

/* void cmd_history(int32_t pnt)
 * Description: Copies the previous command from history into keyboard buffer to show
 * Inputs: pnt
 * Return Value: None
 * Side Effects: Displaces the screen position and updates the keyboard buffer, putc's the previous buffer to screen
 */
void cmd_history(int32_t pnt){

    // Get length of the command in the history
    uint32_t hist_len = prev_len[pnt][shown_tid];

    strncpy(KB_BUF[shown_tid], prev_buf[pnt][shown_tid], hist_len);

    uint32_t i;

    displace_screen_pos((-1*char_ct[shown_tid]), 0, shown_tid);

    for(i = 0; i < char_ct[shown_tid]; i++){
        putc(' ', shown_tid);
    }

    displace_screen_pos((-1*char_ct[shown_tid]), 0, shown_tid);

    for(i = 0; i < hist_len; i++){
        putc(prev_buf[pnt][shown_tid][i], shown_tid);
    }

    char_ct[shown_tid] = hist_len;
    cursor_pos[shown_tid] = hist_len;
}

/* void shift_cmd_history_up()
 * Description: Shifts the command history by 1 so the previous buffer contains its previous command
 * Inputs: None
 * Return Value: None
 * Side Effects: Copies the i-1 idx into the i idx for the previous command buffer
 */
void shift_cmd_history_up(){
    uint32_t i;

    hist_max[shown_tid]++;

    for(i = NUM_PREVS-1; i > 0; i--){
        memcpy(prev_buf[i][shown_tid], prev_buf[i-1][shown_tid], 128); // copies 128 bytes for the full size of the buffer
        prev_len[i][shown_tid] = prev_len[i-1][shown_tid];
    }
}


/* void default_kb_handler()
 * Description: Handles inputs from keyboard and stores in buffer for terminal to read/write
 * Inputs: None
 * Return Value: None
 * Side Effects: Chars written to buffer
 */
void default_kb_handler() {
    uint8_t scan_code;
    cli();
    scan_code = inb(KB_IN_PORT);
  
    // First Section: Checking for modifiers (CAPS, SHIFT, CTRL)
    // if modifier is the scancode we just end intr and return 
    if(ENTER_PRESSED[shown_tid]){ // DO NOTHING IF TERMINAL HAS NOT TAKEN THE DATA FROM READ YET
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == CAPS_LOCK){ // checking if caps is set, if its already high turn toggle off
        if(caps_flag) caps_flag = 0;
        else caps_flag = 1;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == CTRL_PRESSED){ 
        ctrl_flag = 1; // checking if ctrl_pressed or released, sets flag
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == CTRL_RELEASED){ // if ctrl no longer pressed we reset flag
        ctrl_flag = 0;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }
    
    if(scan_code == SHIFT_L || scan_code == SHIFT_R) { // checks if shift pressed or released, sets flag
        shift_flag = 1;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == SHIFT_R_RELEASED || scan_code == SHIFT_L_RELEASED) { // resets the flag if shifts aren't held
        shift_flag = 0;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if (scan_code == BACKSPACE_PRESSED){
        // Only do backspace if we have something typed
        if(char_ct[shown_tid] > 0){
            int32_t j;
            if(cursor_pos[shown_tid] != char_ct[shown_tid]){
                for(j = cursor_pos[shown_tid] - 1; j < char_ct[shown_tid]; j++){
                    KB_BUF[shown_tid][j] = KB_BUF[shown_tid][j + 1];
                    KB_BUF[shown_tid][j + 1] = NULL;
                }
                char_ct[shown_tid]--;
                cursor_pos[shown_tid]--;
                j = 0;
                displace_screen_pos(-1*(cursor_pos[shown_tid] + 1), 0, shown_tid);
                while(j < char_ct[shown_tid] + 1){
                    putc(KB_BUF[shown_tid][j], shown_tid);
                    j++;
                }
                displace_screen_pos(-1*(char_ct[shown_tid] - (cursor_pos[shown_tid] - 1)), 0, shown_tid);
                displace_cursor(-1*(char_ct[shown_tid] - (cursor_pos[shown_tid] - 1)), 0);
            }
            else{
                KB_BUF[shown_tid][char_ct[shown_tid]] = NULL;
                char_ct[shown_tid]--;
                cursor_pos[shown_tid] = char_ct[shown_tid];
                moveScreenPosBack(); // resets the cursor
                putc(0, shown_tid);
                moveScreenPosBack();
                displace_cursor(-1, 0);
            }
        }
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == LEFT_ARROW){
        if(cursor_pos[shown_tid] > 0 && cursor_pos[shown_tid] <= char_ct[shown_tid] + 1){
            moveScreenPosBack();
            displace_cursor(-1, 0);
            cursor_pos[shown_tid]--;
        }
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == RIGHT_ARROW){
        if(cursor_pos[shown_tid] < char_ct[shown_tid]){
            moveScreenPosForward();
            displace_cursor(1, 0);
            cursor_pos[shown_tid]++;
        }
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == L_ALT_PRESSED){
        alt_flag = 1;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == L_ALT_RELEASED){
        alt_flag = 0;
        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    if(scan_code == UP_PRESSED){
        send_eoi(KB_PIC_PORT);
        if(curr_hist_pnt[shown_tid] < hist_max[shown_tid]){ // -1 because of zero indexing
            curr_hist_pnt[shown_tid]++;
            cmd_history(curr_hist_pnt[shown_tid]);
        }
        sti();
        return;    
    }

    if(scan_code == DOWN_PRESSED){
        send_eoi(KB_PIC_PORT);
        if(curr_hist_pnt[shown_tid] > 0){ // -1 because of zero indexing
            curr_hist_pnt[shown_tid]--;
            cmd_history(curr_hist_pnt[shown_tid]);
        }
        sti();
        return;    
    }

    if (reg_kb_map[scan_code] == '\n' && scan_code < PRESS_LIMIT){ // if we are at a newline scancode
        shift_cmd_history_up();
        strncpy(prev_buf[0][shown_tid], KB_BUF[shown_tid], char_ct[shown_tid]);
        prev_len[0][shown_tid] = char_ct[shown_tid];
        curr_hist_pnt[shown_tid] = -1;

        KB_BUF[shown_tid][char_ct[shown_tid]] = reg_kb_map[scan_code]; // sets newline
        putc(reg_kb_map[scan_code], shown_tid);
        displace_cursor(0, 1);
        char_ct[shown_tid] = 0;
        cursor_pos[shown_tid] = 0;
        ENTER_PRESSED[shown_tid] = 1; // tells terminal end of line char and we need to reset the position

        send_eoi(KB_PIC_PORT);
        sti();
        return;
    }

    // checking ctrl conditions
    if (ctrl_flag){ // checking if ctl-L for clearing screen
        if (scan_code == L_CODE){
            //char_ct = 0;
            clear();
            moveScreenPosStart(); // reset cursor
            puts("Starting 391 Shell\n391OS> ", shown_tid);
            uint32_t i = 0;
            while(i < char_ct[shown_tid]){
                putc((int8_t)(KB_BUF[shown_tid][i]), shown_tid);
                i++;
            }

            send_eoi(KB_PIC_PORT);
            sti();
            return;
        }
    }

    if(alt_flag){
        send_eoi(KB_PIC_PORT);
        // printf("%x\n", scan_code);
        switch (scan_code) {
            case TERM_1:
                term_switch(0);
                break;
            case TERM_2:
                term_switch(1);
                break;
            case TERM_3:
                term_switch(2);
                break;
            default:
                break;
        }
        sti();
        return;
    }

    if ((char_ct[shown_tid] < BUF_LIMIT) && (scan_code < PRESS_LIMIT)){ // 
        if ((scan_code) <= READ_LIMIT) { // is this in the range of chars we care about, 3D is F3 pressed and limit for where we care about pressed
            //checking shift conditions
            if (shift_flag){
                if(caps_flag){
                    if(cursor_pos[shown_tid] == char_ct[shown_tid]){
                        KB_BUF[shown_tid][char_ct[shown_tid]] = reg_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid] = char_ct[shown_tid];
                        putc(reg_kb_map[scan_code], shown_tid);
                    }
                    else{
                        int i;
                        for(i= char_ct[shown_tid]; i >= cursor_pos[shown_tid]; i--){
                            KB_BUF[shown_tid][i + 1] = KB_BUF[shown_tid][i];
                            KB_BUF[shown_tid][i] = NULL;
                        }
                        KB_BUF[shown_tid][cursor_pos[shown_tid]] = reg_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid]++;
                        i = 0;
                        displace_screen_pos(-1*(cursor_pos[shown_tid]-1), 0, shown_tid);
                        while(i < char_ct[shown_tid]){
                            putc(KB_BUF[shown_tid][i], shown_tid);
                            i++;
                        }
                        displace_screen_pos(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0, shown_tid);
                        displace_cursor(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0);
                    }
                } else {                    // if not alphanumeric we just put that value normally
                    if(cursor_pos[shown_tid] == char_ct[shown_tid]){
                        KB_BUF[shown_tid][char_ct[shown_tid]] = shift_kb_map[scan_code]; 
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid] = char_ct[shown_tid];
                        putc(shift_kb_map[scan_code], shown_tid);
                    }
                    else{
                        int i;
                        for(i= char_ct[shown_tid]; i >= cursor_pos[shown_tid]; i--){
                            KB_BUF[shown_tid][i + 1] = KB_BUF[shown_tid][i];
                            KB_BUF[shown_tid][i] = NULL;
                        }
                        KB_BUF[shown_tid][cursor_pos[shown_tid]] = shift_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid]++;
                        i = 0;
                        displace_screen_pos(-1*(cursor_pos[shown_tid]-1), 0, shown_tid);
                        while(i < char_ct[shown_tid]){
                            putc(KB_BUF[shown_tid][i], shown_tid);
                            i++;
                        }
                        displace_screen_pos(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0, shown_tid);
                        displace_cursor(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0);
                    }
                }
            }

            //check if caps is high
            else if (caps_flag){
                if(shift_flag){ // if also shift condition, we use regular kb scancodes
                    if(cursor_pos[shown_tid] == char_ct[shown_tid]){
                        KB_BUF[shown_tid][char_ct[shown_tid]] = reg_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid] = char_ct[shown_tid];
                        putc(reg_kb_map[scan_code], shown_tid);
                    }
                    else{
                        int i;
                        for(i= char_ct[shown_tid]; i >= cursor_pos[shown_tid]; i--){
                            KB_BUF[shown_tid][i + 1] = KB_BUF[shown_tid][i];
                            KB_BUF[shown_tid][i] = NULL;
                        }
                        KB_BUF[shown_tid][cursor_pos[shown_tid]] = reg_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid]++;
                        i = 0;
                        displace_screen_pos(-1*(cursor_pos[shown_tid]-1), 0, shown_tid);
                        while(i < char_ct[shown_tid]){
                            putc(KB_BUF[shown_tid][i], shown_tid);
                            i++;
                        }
                        displace_screen_pos(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0, shown_tid);
                        displace_cursor(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0);
                    }
                } else { // otherwise take from the caps map
                    if(cursor_pos[shown_tid] == char_ct[shown_tid]){
                        KB_BUF[shown_tid][char_ct[shown_tid]] = caps_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid] = char_ct[shown_tid];
                        putc(caps_kb_map[scan_code], shown_tid);
                    }
                    else{
                        int i;
                        for(i = char_ct[shown_tid]; i >= cursor_pos[shown_tid]; i--){
                            KB_BUF[shown_tid][i + 1] = KB_BUF[shown_tid][i];
                            KB_BUF[shown_tid][i] = NULL;
                        }
                        KB_BUF[shown_tid][cursor_pos[shown_tid]] = caps_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid]++;
                        i = 0;
                        displace_screen_pos(-1*(cursor_pos[shown_tid]-1), 0, shown_tid);
                        while(i < char_ct[shown_tid]){
                            putc(KB_BUF[shown_tid][i], shown_tid);
                            i++;
                        }
                        displace_screen_pos(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0, shown_tid);
                        displace_cursor(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0);
                    }
                }
                
            }

            else{ // no modifier, just a regular ascii char we add to buffer
                if(cursor_pos[shown_tid] == char_ct[shown_tid]){
                    KB_BUF[shown_tid][char_ct[shown_tid]] = reg_kb_map[scan_code];
                    char_ct[shown_tid]++;
                    cursor_pos[shown_tid] = char_ct[shown_tid];
                    putc(reg_kb_map[scan_code], shown_tid);
                }
                else{
                        int i;
                        for(i= char_ct[shown_tid]; i >= cursor_pos[shown_tid]; i--){
                            KB_BUF[shown_tid][i + 1] = KB_BUF[shown_tid][i];
                            KB_BUF[shown_tid][i] = NULL;
                        }
                        KB_BUF[shown_tid][cursor_pos[shown_tid]] = reg_kb_map[scan_code];
                        char_ct[shown_tid]++;
                        cursor_pos[shown_tid]++;
                        i = 0;
                        displace_screen_pos(-1*(cursor_pos[shown_tid]-1), 0, shown_tid);
                        while(i < char_ct[shown_tid]){
                            putc(KB_BUF[shown_tid][i], shown_tid);
                            i++;
                        }
                        displace_screen_pos(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0, shown_tid);
                        displace_cursor(-1*(char_ct[shown_tid] - cursor_pos[shown_tid]), 0);
                    }
            }
        }
    }

    send_eoi(KB_PIC_PORT);
    sti();
    return;
}

