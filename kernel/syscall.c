#include "syscall.h"
#include "vga.h"
#include "idt.h"

// Forward declaration of assembly wrapper
extern void syscall_handler_asm(void);

void syscall_handler(registers_t regs) {
    // EAX contains the syscall number
    uint32_t syscall_num = regs.eax;
    
    // Simple verification
    // vga_print("Syscall received: ");
    // char num[16];
    // int_to_string(syscall_num, num); // Assume we have this or similar
    // vga_print(num);
    // vga_print("\n");

    switch (syscall_num) {
        case 0: // TEST / PRINT
            vga_print((char*)regs.ebx); // EBX = String pointer
            break;
        case 1: // EXIT (Example)
            vga_print("Syscall Exit called.\n");
            break;
        default:
            vga_print("Unknown syscall.\n");
            break;
    }
}

void init_syscalls(void) {
    vga_print("Initializing System Calls...\n");
    // Register INT 0x80
    // Selector: 0x08 (Kernel Code)
    // Flags: 0xEE (Present | Ring 3 | 32-bit Interrupt Gate)
    // This allows Ring 3 code to call INT 0x80
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEE);
}
