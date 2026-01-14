#include "vga.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"

void kernel_main(void) {
    // Initialize VGA display
    vga_init();
    
    // Initialize interrupts
    idt_init();
    
    // Initialize timer
    init_timer(50);
    
    // Initialize keyboard
    keyboard_init();
    
    // Enable interrupts
    __asm__ volatile("sti");
    
    // Initialize and run shell
    shell_init();
    shell_run();
    
    // Infinite loop (should never reach here)
    while (1) {
        __asm__ volatile("hlt");
    }
}
