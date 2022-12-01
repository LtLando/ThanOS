#ifndef _EXCEPTION_HANDLER_H
#define _EXCEPTION_HANDLER_H

#include "types.h"

#define TICK_PER_SEC 300
#define RTC_REG_C 0x8C
#define RTC_PORT_CMD 0x70
#define RTC_PORT_DATA 0x71
#define RTC_PIC_PORT 0x8
#define NUM_SCAN_CHARS 128
#define KB_PIC_PORT 0x1
#define KB_PORT_DATA 0x60

// Overall Handlers
extern void do_irq(unsigned int idx);

// Interrupt handlers
void handle_rtc(void);
void handle_keyboard(void);
void default_kb_handler(void);
void default_rtc_handler(void);

extern void set_rtc_interupt_rate(unsigned int rate);

#define except_handle_gen(err_name, func_name)   \
    void func_name(){                            \
        clear();                                 \
        set_screen_pos(0, 0);                    \
        printf(""#err_name"\n System Halting!"); \
        asm volatile ("hlt;"); }


// Exception Handlers
void zero_divide_handle(void);
void debug_handle(void);
void nmi_handle(void);
void breakpoint_handle(void);
void overflow_handle(void);
void bound_handle(void);
void invalid_op_handle(void);
void device_not_avail_handle(void);
void double_handle(void);
void inval_tss_handle(void);
void seg_not_pres_handle(void);
void stack_fault_handle(void);
void gen_protect_handle(void);
void page_fault_handle(void);
void float_error_handle(void);
void align_check_handle(void);
void machine_check_handle(void);
void simd_fp_handle(void);

#endif
