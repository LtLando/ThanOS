#include "x86_desc.h"
#include "idt.h"
#include "lib.h"


/* void build_idt_entry(uint8_t vector, void* isr)
 * Description: Create an individual IDT entry
 * Inputs: None
 * Return Value: None
 * Side Effects: Changes memory in the IDT table
 */
void build_idt_entry(uint8_t vector, void* isr) {
    SET_IDT_ENTRY(idt[vector], isr); // Set the offset fields to the function address (isr)
    idt[vector].seg_selector = KERNEL_CS; // third byte in GDT
    idt[vector].present = 0x1; // Interrupts are present thus 1
    idt[vector].dpl = 0x0; // privilege level 0 (kernel) for exp and ints
    idt[vector].reserved0 = 0x0; // | Defined by x86 intel manual
    idt[vector].reserved1 = 0x1; // |
    idt[vector].reserved2 = 0x1; // |
    idt[vector].reserved3 = 0x0; // V
    idt[vector].reserved4 = 0x0; //---
    idt[vector].size = 0x1; // 32 bit descriptors = 1, 16 bit = 0
}


/* void build_syscall_entry(uint8_t vector, void* isr)
 * Description: Create an idt entry for system calls
 * Inputs: None
 * Return Value: None
 * Side Effects: Changes memory in the IDT table
 */
void build_syscall_entry(uint8_t vector, void* isr) {
    SET_IDT_ENTRY(idt[vector], isr); // Set the offset fields to the function address (isr)
    idt[vector].seg_selector = KERNEL_CS; // third byte in GDT
    idt[vector].present = 0x1; // Interrupts are present thus 1
    idt[vector].dpl = 0x3; // privilege level 3 (user + kernel) for system calls
    idt[vector].reserved0 = 0x0; // | Defined by x86 intel manual
    idt[vector].reserved1 = 0x1; // |
    idt[vector].reserved2 = 0x1; // |
    idt[vector].reserved3 = 0x1; // V
    idt[vector].reserved4 = 0x0; //---
    idt[vector].size = 0x1; // 32 bit descriptors = 1, 16 bit = 0
}


/* extern void init_idt()
 * Description: Initialize the IDT
 * Inputs: None
 * Return Value: None
 * Side Effects: Changes memory in the IDT table
 */
extern void init_idt(){
    // Build and fill the IDT with the following handlers
    build_idt_entry(0, int0_handler);
    build_idt_entry(1, int1_handler);
    build_idt_entry(2, int2_handler);
    build_idt_entry(3, int3_handler);
    build_idt_entry(4, int4_handler);
    build_idt_entry(5, int5_handler);
    build_idt_entry(6, int6_handler);
    build_idt_entry(7, int7_handler);
    build_idt_entry(8, int8_handler);
    build_idt_entry(9, int9_handler);
    build_idt_entry(10, int10_handler);
    build_idt_entry(11, int11_handler);
    build_idt_entry(12, int12_handler);
    build_idt_entry(13, int13_handler);
    build_idt_entry(14, int14_handler);
    build_idt_entry(15, int15_handler);
    build_idt_entry(16, int16_handler);
    build_idt_entry(17, int17_handler);
    build_idt_entry(18, int18_handler);
    build_idt_entry(19, int19_handler);

    build_idt_entry(32, int32_handler);
    build_idt_entry(33, int33_handler);
    build_idt_entry(40, int40_handler);

    build_syscall_entry(0x80, syscall_handler);

    // Load the IDTR with the location of the IDT
    lidt(idt_desc_ptr);
}

