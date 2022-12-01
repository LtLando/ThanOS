#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "types.h"

#define PIMAGE_VADDR 0x08048000
#define PIMAGE_VADDR_BASE 0x08400000
#define PCB_OFF 0x4
#define PER_TERM_ROOT_PROC_ID -1

#define PROG_VIDEO_BASE PIMAGE_VADDR_BASE+MB_4_SIZE

extern int32_t handle_syscall0(uint32_t arg1, uint32_t arg2, uint32_t arg3);

int32_t handle_exec_shell(uint8_t tid);

extern int32_t handle_syscall1(uint32_t arg1, uint32_t arg2, uint32_t arg3);

extern int32_t handle_syscall2(uint32_t arg1, uint32_t arg2, uint32_t arg3);

int32_t change_page(uint32_t arg);

#endif
