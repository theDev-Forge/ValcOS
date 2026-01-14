#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "idt.h"

void init_syscalls(void);
void syscall_handler(registers_t regs);

#endif
