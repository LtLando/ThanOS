/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

/*
 * NOTE: VERY GOOD REFERENCE FOR IRQ INTERACTION
 *      https://wiki.osdev.org/8259_PIC
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */


/* void i8259_init(void)
 * Description: Initialize the 8259 PIC
 * Inputs: None
 * Return Value: None
 * Side Effects: Bytes written to and read from PICs
 */
void i8259_init(void) {
    // Initialize the global master and slave masks they will always
    // hold the current status of the masks for their respective PICs
    // 0xFF Masks Out All Interrupts
    master_mask = 0xFF;
    slave_mask = 0xFF;

    // sequence for writing control words to PIC
    outb(ICW1, MASTER_8259_PORT_CMD);
    outb(ICW1, SLAVE_8259_PORT_CMD);

    outb(ICW2_MASTER, MASTER_8259_PORT_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_PORT_DATA);

    outb(ICW3_MASTER, MASTER_8259_PORT_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_PORT_DATA);

    outb(ICW4, MASTER_8259_PORT_DATA);
    outb(ICW4, SLAVE_8259_PORT_DATA);

    outb(master_mask, MASTER_8259_PORT_DATA);
    outb(slave_mask, SLAVE_8259_PORT_DATA);

    enable_irq(MASTER_CAS_PORT);
}


/* void disable_irq(uint32_t irq_num)
 * Description: Unmask the specified IRQ on the PIC Device
 * Inputs: unint32_t irq_num = IRQ to unmask
 * Return Value: None
 * Side Effects: Changes Masks On PIC device
 */
void disable_irq(uint32_t irq_num) {
    if(irq_num < 8){
        master_mask |= (1 << irq_num);
        outb(master_mask, MASTER_8259_PORT_DATA);
    } else {
        slave_mask |= (1 << (irq_num - NUM_PORTS));
        outb(slave_mask, SLAVE_8259_PORT_DATA);
    }
}


/* void enable_irq(uint32_t irq_num)
 * Description: Mask the specified IRQ on the PIC Device
 * Inputs: unint32_t irq_num = IRQ to mask
 * Return Value: None
 * Side Effects: Changes Masks On PIC device
 */
void enable_irq(uint32_t irq_num) {
    if(irq_num < NUM_PORTS){
        master_mask &= ~(1 << irq_num);
        outb(master_mask, MASTER_8259_PORT_DATA);
    } else {
        slave_mask &= ~(1 << (irq_num - NUM_PORTS));
        outb(slave_mask, SLAVE_8259_PORT_DATA);
    }
}

    
/* void send_eoi(uint32_t irq_num)
 * Description: Send end-of-interrupt signal for the specified IRQ
 * Inputs: unint32_t irq_num = IRQ to deliver EOI about
 * Return Value: None
 * Side Effects: Bytes written to PIC device(s)
 */
void send_eoi(uint32_t irq_num) {
	if(irq_num < NUM_PORTS){
        outb((EOI | irq_num), MASTER_8259_PORT_CMD);
    } else {
        outb((EOI | MASTER_CAS_PORT), MASTER_8259_PORT_CMD);
        outb((EOI | (irq_num - NUM_PORTS)), SLAVE_8259_PORT_CMD);
    }
}
