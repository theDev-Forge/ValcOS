#include "timer.h"
#include "idt.h"
#include "vga.h"

uint32_t tick = 0;

// Unused callback removed

// Forward declaration for the assembly handler
extern void timer_handler_asm(void);

// This will be called from the assembly wrapper
void timer_handler(void) {
    tick++;
    
    // Every 50 ticks (approx 1 second at 50Hz)
    if (tick % 50 == 0) {
        // vga_print("Tick "); // Visual verification disabled
    }

    // Send EOI to PIC (Master only as IRQ0 is on master)
    outb(0x20, 0x20);
}

void init_timer(uint32_t frequency) {
    // Register timer handler
    idt_set_gate(32, (uint32_t)timer_handler_asm, 0x08, 0x8E);

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency.
    uint32_t divisor = 1193180 / frequency;

    // Send the command byte.
    outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
}

void timer_wait(int ticks) {
    unsigned long eticks;
    eticks = tick + ticks;
    while(tick < eticks);
}
