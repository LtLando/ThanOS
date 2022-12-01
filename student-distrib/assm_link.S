#define ASM 1

# Macro to create Assembly Linkage functions for different interrupts
#define ASM_LINKAGE(name, idx)  \
    name:                       \
        pushal;                 \
        pushfl;                 \
        pushl $idx;             \
        call general_link;      \
        addl $4, %esp;          \
        popfl;                  \
        popal;                  \
        iret;                   \


# Declare interrupt handlers global so idt.c can see them
.globl int0_handler
.globl int1_handler
.globl int2_handler
.globl int3_handler
.globl int4_handler
.globl int5_handler
.globl int6_handler
.globl int7_handler
.globl int8_handler
.globl int9_handler
.globl int10_handler
.globl int11_handler
.globl int12_handler
.globl int13_handler
.globl int14_handler
.globl int15_handler
.globl int16_handler
.globl int17_handler
.globl int18_handler
.globl int19_handler

.globl int32_handler
.globl int33_handler
.globl int40_handler
.globl syscall_handler

.globl sys_halt
.globl sys_execute
.globl sys_read
.globl sys_write
.globl sys_open
.globl sys_close
.globl get_args


syscall_jumptable: .long sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, get_args, sys_vidmap # TODO: FILL JUMP TABLE WITH SPECFICI SYSCALL HANDLERS


/* intn_handler
 * 
 * Description: Linkage function to push the interrupt vector and call general handler
 * Inputs: None
 * Outputs: None
 * Returns: None
 * Side Effects: Calls the general_link assembly general interrupt handler
 */

# Create the functions using the macro
ASM_LINKAGE(int0_handler, 0)
ASM_LINKAGE(int1_handler, 1)
ASM_LINKAGE(int2_handler, 2)
ASM_LINKAGE(int3_handler, 3)
ASM_LINKAGE(int4_handler, 4)
ASM_LINKAGE(int5_handler, 5)
ASM_LINKAGE(int6_handler, 6)
ASM_LINKAGE(int7_handler, 7)
ASM_LINKAGE(int8_handler, 8)
ASM_LINKAGE(int9_handler, 9)
ASM_LINKAGE(int10_handler, 10)
ASM_LINKAGE(int11_handler, 11)
ASM_LINKAGE(int12_handler, 12)
ASM_LINKAGE(int13_handler, 13)
ASM_LINKAGE(int14_handler, 14)
ASM_LINKAGE(int15_handler, 15)
ASM_LINKAGE(int16_handler, 16)
ASM_LINKAGE(int17_handler, 17)
ASM_LINKAGE(int18_handler, 18)
ASM_LINKAGE(int19_handler, 19)

ASM_LINKAGE(int32_handler, 32)
ASM_LINKAGE(int33_handler, 33)
ASM_LINKAGE(int40_handler, 40)


/* syscall_handler
 * 
 * Description: Link system call assembly to C handling chain, this
                step pushes flags and does the return from interrupt
 * Inputs: None
 * Outputs: None
 * Returns: None
 * Side Effects: Calls the syscall_link linkage
 */
syscall_handler:

    addl $-1, %eax

    cmpl $0, %eax
    jl syscall_handler_done_bad
    cmpl $10, %eax
    jg syscall_handler_done_bad

    # Protect all regs as this is an interrupt
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    pushfl
    
    pushl %edx
    pushl %ecx
    pushl %ebx
    call *syscall_jumptable(, %eax, 4) # Jump to handler at: table_start + 4*(syscall #)
    addl $12, %esp # tear arg off stack, 12 for 3 args

    popfl
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx

    jmp syscall_handler_done

syscall_handler_done_bad:
    movl $-1, %eax
syscall_handler_done:
    iret


/* general_link
 * 
 * Description: General handler for interrupts, the linkage handlers defined above
                will push the index of the interrupt onto the stack which we then
                push into the stack again as an argument for the C interrupt handler
                to use in order to decide what action to take
 * Inputs: interrupt_vector: int -> index of interrupt in the IDT
 * Outputs: None
 * Returns: None
 * Side Effects: Calls the do_irq C general interrupt handler
 */
general_link:
    pushl %ebp
    movl %esp, %ebp
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    pushl %esi

    movl 8(%ebp), %ebx # ebp + 8 = first arg
    pushl %ebx 
    call do_irq
    addl $4, %esp # tear arg off stack, 4 for 1 arg

    popl %esi
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax
    leave
    ret

