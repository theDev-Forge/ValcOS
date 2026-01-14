#include "vga.h"
#include "gdt.h"
#include "tss.h"
#include "idt.h"
#include "syscall.h"
#include "timer.h"
#include "memory.h"
#include "fat12.h"
#include "process.h"
#include "shell.h" // Keep shell.h for shell_init and shell_run
#include "keyboard.h" // Keep keyboard.h for shell_init (which might use it)

void kernel_main(void) {
    // Initialize VGA display
    vga_init();
    
    // Initialize GDT (Global Descriptor Table)
    init_gdt();
    
    // Initialize TSS (Task State Segment)
    init_tss();
    
    // Initialize interrupts
    idt_init();
    
    // Initialize System Calls
    init_syscalls();
    
    // Initialize timer
    init_timer(50);
    
    // Initialize memory
    memory_init();
    
    // Initialize FAT12 (Ramdisk)
    fat12_init();
    fat12_list_directory();
    
    // Initialize Multitasking
    process_init();
    
    // Enable interrupts (Starts Multitasking via Timer)
    __asm__ volatile("sti");
    
    // Initialize and run shell (Main Task)
    keyboard_init(); // Re-add keyboard_init as shell might need it
    shell_init();
    shell_run();
}
