#include "pit.h"
#include "i8259.h"
#include "lib.h"
#include "procs.h"
#include "rtc.h"

uint32_t sleep_counter = 0;
uint32_t pit_ticks;
uint8_t sched_en;

/* void init_pit()
 * Description: Initializes the PIT
 * Inputs: None
 * Return Value: None
 * Side Effects: sets the PIT interval, initializes the device and enables irq
 */
void init_pit(){
    outb(PIT_INIT_WORD, PIT_CMD);

    outb((PIT_INTERVAL & 0xFF), PIT_DATA);
    outb(((PIT_INTERVAL >> 8) & 0xFF), PIT_DATA); // shifted 8 bits to fill whole 16 bit

    enable_irq(PIT_IRQ);
}

/* void handle_pit()
 * Description: Handles the PIT interrupt and schedules next process
 * Inputs: None
 * Return Value: None
 * Side Effects: calls schedule next to handle process switching
 */
void handle_pit(){
    cli();
    send_eoi(PIT_IRQ);

    sleep_counter++;

    default_rtc_handler();

    if(sched_en) sched_next();

    sti();
}


void sleep(uint32_t count){
    uint32_t curr = sleep_counter;

    while((curr + count) > sleep_counter){

    }

    return;
}
