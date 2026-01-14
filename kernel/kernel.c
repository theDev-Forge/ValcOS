#include "vga.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"
#include "memory.h"
#include "fat12.h"

void kernel_main(void) {
    // Initialize VGA display
    vga_init();
    
    // Initialize interrupts
    idt_init();
    
    // Initialize timer
    init_timer(50);
    
    // Enable interrupts (Required for Floppy/Timer)
    __asm__ volatile("sti");
    
    // Initialize memory
    memory_init();
    
    // Initialize FAT12 (Requires interrupts)
    fat12_init();
    fat12_list_directory();
    
    // Initialize keyboard
    keyboard_init();
    
    // Initialize and run shell
    shell_init();
    shell_run();
    
    // Infinite loop (should never reach here)
    while (1) {
        __asm__ volatile("hlt");
    }
}
