#include "exception_handler.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "keyboard.h"
#include "pit.h"

// pointers to drivers for keyboard and rtc
void (*keyboard_handler)(void) = &default_kb_handler;
void (*rtc_handler)(void) = &default_rtc_handler;

// Table to convert PS2 Scancodes to ASCII Charecters
char scan_to_ascii [128] = {
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

/* extern void do_irq(unsigned int idx)
 * Description: Use interrupt vector to call the requested specific interrupt handler
 * Inputs: unint32_t idx = interrupt vector
 * Return Value: None
 * Side Effects: Calls specific interrupt handlers
 */
extern void do_irq(unsigned int idx) {
    cli();
    //printf("An interrupt of index %d has occurred!\n", idx);

    // Cases are interrupt vectors
    switch (idx) {
        case 0x0:
            zero_divide_handle();
            break;
        case 0x1:
            debug_handle();
            break;
        case 0x2:
            nmi_handle();
            break;
        case 0x3:
            breakpoint_handle();
            break;
        case 0x4:
            overflow_handle();
            break;
        case 0x5:
            bound_handle();
            break;
        case 0x6:
            invalid_op_handle();
            break;
        case 0x7:
            device_not_avail_handle();
            break;
        case 0x8:
            double_handle();
            break;
        case 0xA:
            inval_tss_handle();
            break;
        case 0xB:
            seg_not_pres_handle();
            break;
        case 0xC:
            stack_fault_handle();
            break;
        case 0xD:
            gen_protect_handle();
            break;
        case 0xE:
            page_fault_handle();
            break;
        case 0x10:
            float_error_handle();
            break;
        case 0x11:
            align_check_handle();
            break;
        case 0x12:
            machine_check_handle();
            break;
        case 0x13:
            simd_fp_handle();
            break;
        case 0x20:
            handle_pit();
            break;
        case 0x21: 
            handle_keyboard();
            break;
        case 0x28:        
            handle_rtc();
            break;
        default:
            break;
    }
    sti();
}

// Generate Exception Handlers
except_handle_gen(Divide by Zero Exception, zero_divide_handle);
except_handle_gen(Debug Exception, debug_handle);
except_handle_gen(Breakpoint Exception, breakpoint_handle);
except_handle_gen(Non-Maskable Interrupt, nmi_handle);
except_handle_gen(Overflow Exception, overflow_handle);
except_handle_gen(Bound Range Exceeded Exception, bound_handle);
except_handle_gen(Invalid Opcode Exception, invalid_op_handle);
except_handle_gen(Device Not Available Exception, device_not_avail_handle);
except_handle_gen(Double Fault Exception, double_handle);
except_handle_gen(Invalid TSS Exception, inval_tss_handle);
except_handle_gen(Segment Not Present, seg_not_pres_handle);
except_handle_gen(Stack Fault Exception, stack_fault_handle);
except_handle_gen(General Protection Exception, gen_protect_handle);
// except_handle_gen(Page-Fault Exception, page_fault_handle);
except_handle_gen(x87 FPU Floating-Point Error, float_error_handle);
except_handle_gen(Alignment Check Exception, align_check_handle);
except_handle_gen(Machine-Check Exception, machine_check_handle);
except_handle_gen(SIMD Floating-Point Exception, simd_fp_handle);

void page_fault_handle(){
    uint32_t i;
    asm("\t movl %%cr2,%0" : "=r"(i));
    printf("Page Fault when trying to access 0x%x\n System Halting!", i);
    asm volatile ("hlt;");
}


/* void handle_rtc()
 * Description: Handle a Real Time Clock Interrupt
 * Inputs: None
 * Return Value: None
 * Side Effects: Calls the currently installed RTC Driver
 */
void handle_rtc(){
    if(handle_rtc != NULL){
        (*rtc_handler)();
    }
}


/* void handle_keyboard()
 * Description: Handle a keyboard interrupt
 * Inputs: None
 * Return Value: None
 * Side Effects: Calls the currently installed Keyboard Driver
 */
void handle_keyboard(){
    if(keyboard_handler != NULL){
        keyboard_handler();
    }
}
