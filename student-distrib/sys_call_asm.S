.globl test_sys_call

test_sys_call:
    pushl %ebp
    movl %esp, %ebp

    pushl %ebx
    pushl %edi
    pushl %esi

    andl $0, %eax # TEST 0th system call
    movl $10, %ebx
    movl $20, %ecx
    movl $30, %edx

    int $0x80 # Call syscall interrupts

    popl %esi
    popl %edi
    popl %ebx

    leave
    ret
