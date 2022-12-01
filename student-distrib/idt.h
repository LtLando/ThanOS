#ifndef _IDT_H
#define _IDT_H

/* Indentify interupt handlers - Defined in assm_link.S */
extern void int0_handler(int idx);
extern void int1_handler(int idx);
extern void int2_handler(int idx);
extern void int3_handler(int idx);
extern void int4_handler(int idx);
extern void int5_handler(int idx);
extern void int6_handler(int idx);
extern void int7_handler(int idx);
extern void int8_handler(int idx);
extern void int9_handler(int idx);
extern void int10_handler(int idx);
extern void int11_handler(int idx);
extern void int12_handler(int idx);
extern void int13_handler(int idx);
extern void int14_handler(int idx);
extern void int15_handler(int idx);
extern void int16_handler(int idx);
extern void int17_handler(int idx);
extern void int18_handler(int idx);
extern void int19_handler(int idx);
extern void int20_handler(int idx);

extern void int32_handler(int idx);
extern void int33_handler(int idx);
extern void int40_handler(int idx);

extern void syscall_handler();

extern void init_idt(void);

#endif
