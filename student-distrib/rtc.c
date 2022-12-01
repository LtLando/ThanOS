#include "rtc.h"
#include "exception_handler.h"
#include "lib.h"
#include "i8259.h"
#include "term_helper.h"


// Curr tick count value
uint32_t tick_counter[3] = {0, 0, 0};
uint32_t gl_rtc_rate[3] = {1, 1, 1};

// Flag of interrupt arrived for RTC read()
volatile uint8_t rtc_interrupt_arrived[3] = {0, 0, 0};

extern unsigned char curr_tid;


/* void default_rtc_handler();
 * Description: If rate is 0, do nothing, else increment tick count and if we hit our designated
 *              tick count, trigger the action of our interrupt
 * Inputs: None
 * Return Value: None
 * Side Effects: Gets a byte from the RTC, may write to video memory
 */
void default_rtc_handler(){
    outb(RTC_REG_C, RTC_PORT_CMD);	// Select register C
    inb(RTC_PORT_DATA);		        // just throw away contents

    if(gl_rtc_rate[curr_tid] == 0){ // Disable Rate = 0
        tick_counter[curr_tid] = 0; // Keep tick count at 0
    }
    else{
        uint32_t i;
        for(i = 0; i < MAX_TERMS; i++){
            if(tick_counter[i] < (TICK_PER_SEC/(gl_rtc_rate[i]))){
                tick_counter[i]++;
            }
            else{
                rtc_interrupt_arrived[i] = 1;
                tick_counter[i] = 0;
            }
        }
    }
    send_eoi(RTC_PIC_PORT);
}


/* extern void set_rtc_interupt_rate(unsigned int rate)
 * Description: Change the desired interupt rate (in interrupts per second)
 * Inputs: None
 * Return Value: None
 * Side Effects: Changes global variable 'gl_rtc_rate'
 */
extern void set_rtc_interupt_rate(unsigned int rate){
    if(rate <= TICK_PER_SEC){
        gl_rtc_rate[curr_tid] = rate;
    }
}



/* void rtc_init()
 * Description: Initializes the Real Time Clock device
 * Inputs: None
 * Return Value: None
 * Side Effects: Bytes written to and read from RTC, Bytes written to PIC
 */
void rtc_init() {

    cli();		           	                // disable interrupts

    outb(RTC_REG_B, RTC_CMD_PORT);		    // select register B, and disable NMI
    char prev = inb(RTC_DATA_PORT);	        // read the current value of register B
    outb(RTC_REG_B, RTC_CMD_PORT);		    // set the index again (a read will reset the index to register D)
    outb(prev | BIT6_ON, RTC_DATA_PORT);    // write the previous value ORed with 0x40. This turns on bit 6 of register B
    enable_irq(RTC_PIC_PORT);

    outb(RTC_REG_A, RTC_CMD_PORT);
    outb(RTC_FREQ_CONST, RTC_DATA_PORT);

    sti();                                  // enable interrupts
}


/* uint32_t rtc_open()
 * Description: Open function for the RTC device 
 * Inputs: None
 * Return Value: 0
 * Side Effects: Sets the RTC interrupt frequency to 2Hz
 */

int32_t rtc_open(const int8_t* filename){
    // Set to 2Hz interrupt rate
    set_rtc_interupt_rate(2);

    return 0;
}


/* uint32_t rtc_close()
 * Description: Close function for the RTC device 
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t rtc_close(int32_t fd){
    return 0;
}


/* uint32_t rtc_read()
 * Description: Read function for the RTC device, blocks until interrupt arrives
 * Inputs: None
 * Return Value: 0
 * Side Effects: None
 */

int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    rtc_interrupt_arrived[curr_tid] = 0;

    while(rtc_interrupt_arrived[curr_tid] == 0){
        // BLOCK while waiting for interrupt
    }

    return 0;
}


/* uint32_t rtc_write()
 * Description: Write function for the RTC device, changes the frequency of interrupts
                to the argument passed but will only accept powers of 2 less than 1024;
 * Inputs: uint32_t* freq_ptr = pointer to value holding frequency of interrupts, must be power of 2, less than 1024;
 * Return Value: 0 for success, -1 for parameter invalidation
 * Side Effects: Changes RTC interrupt rate
 */

int32_t rtc_write(int32_t fd, void* buf, int32_t nbytes){
    if(buf == NULL){
        return -1;
    }

    int32_t freq = *((int32_t*)buf);

    // Reject frequencies greater than 1024, and not powers of 2
    if(freq > RTC_MAX_FREQ){
        return -1;
    }

    set_rtc_interupt_rate(freq);

    return 0;
}

