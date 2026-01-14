#include "syscall.h"
#include "vga.h"
#include "idt.h"
#include "keyboard.h"

// Forward declaration of assembly wrapper
extern void syscall_handler_asm(void);



void syscall_handler(registers_t *regs) {
    // EAX contains the syscall number
    uint32_t syscall_num = regs->eax;
    
    // DEBUG: Verify syscall
    /*
    vga_print("SC: ");
    char buf[16];
    int_to_string(syscall_num, buf);
    vga_print(buf);
    vga_print(" EBX: ");
    int_to_string(regs->ebx, buf); // Assuming int_to_string
    vga_print(buf); // You need to implement int_to_string or hex print
    vga_print("\n");
    */

    switch (syscall_num) {
        case 0: // PRINT (Example: EBX=string)
            vga_print((char*)regs->ebx); 
            break;
        case 1: // EXIT (Example)
            vga_print("Syscall Exit called.\n");
            // Infinite loop
            while(1);
            break;
        case 3: // READ (EBX=fd, ECX=buf, EDX=count)
            // fd 0 = stdin
            if (regs->ebx == 0) {
                 char *buf = (char*)regs->ecx;
                 // Blocking read
                 char c = keyboard_getchar();
                 *buf = c;
                 
                 // Return bytes read (1) in EAX
                 regs->eax = 1;
            } else {
                regs->eax = -1; // Error
            }
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
    // Flags: 0xEF (Present | Ring 3 | 32-bit Trap Gate)
    // Trap Gate (0xF) does NOT disable interrupts, allowing keyboard to work.
    idt_set_gate(0x80, (uint32_t)syscall_handler_asm, 0x08, 0xEF);
}
