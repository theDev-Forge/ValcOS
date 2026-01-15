#include "timer.h"
#include "idt.h"
#include "vga.h"
#include "process.h"

// Timer state
static uint32_t tick = 0;
static uint32_t timer_frequency = 0;
static uint32_t callbacks_executed = 0;

// Callback system
#define MAX_CALLBACKS 16

typedef struct {
    timer_callback_t callback;
    uint32_t interval;
    uint32_t next_tick;
    int active;
} timer_callback_entry_t;

static timer_callback_entry_t callbacks[MAX_CALLBACKS];

// Forward declaration for the assembly handler
extern void timer_handler_asm(void);

// This will be called from the assembly wrapper
void timer_handler(void) {
    tick++;
    
    // Send EOI to PIC (Master only as IRQ0 is on master)
    // Must be done BEFORE schedule() so the PIC can register new interrupts
    outb(0x20, 0x20);

    // Process callbacks
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i].active && tick >= callbacks[i].next_tick) {
            callbacks[i].callback();
            callbacks[i].next_tick = tick + callbacks[i].interval;
            callbacks_executed++;
        }
    }

    // Schedule every 10 ticks (approx 200ms at 50Hz, or 10ms at 1000Hz)
    if (tick % 10 == 0) {
        schedule();
    }
}

void init_timer(uint32_t frequency) {
    timer_frequency = frequency;
    
    // Initialize callback table
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        callbacks[i].active = 0;
        callbacks[i].callback = 0;
        callbacks[i].interval = 0;
        callbacks[i].next_tick = 0;
    }
    
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

uint32_t timer_get_ticks(void) {
    return tick;
}

uint32_t timer_get_uptime_ms(void) {
    if (timer_frequency == 0) return 0;
    // Convert ticks to milliseconds
    return (tick * 1000) / timer_frequency;
}

void timer_wait(int ticks) {
    unsigned long eticks;
    eticks = tick + ticks;
    while(tick < eticks);
}

void timer_sleep_ms(uint32_t ms) {
    if (timer_frequency == 0) return;
    // Convert milliseconds to ticks
    uint32_t ticks_to_wait = (ms * timer_frequency) / 1000;
    if (ticks_to_wait == 0) ticks_to_wait = 1; // At least 1 tick
    timer_wait(ticks_to_wait);
}

int timer_register_callback(timer_callback_t callback, uint32_t interval) {
    if (!callback || interval == 0) return -1;
    
    // Find free slot
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!callbacks[i].active) {
            callbacks[i].callback = callback;
            callbacks[i].interval = interval;
            callbacks[i].next_tick = tick + interval;
            callbacks[i].active = 1;
            return i;
        }
    }
    
    return -1; // No free slots
}

void timer_unregister_callback(int callback_id) {
    if (callback_id >= 0 && callback_id < MAX_CALLBACKS) {
        callbacks[callback_id].active = 0;
        callbacks[callback_id].callback = 0;
    }
}

void timer_get_stats(uint32_t *total_ticks, uint32_t *callbacks_exec) {
    if (total_ticks) *total_ticks = tick;
    if (callbacks_exec) *callbacks_exec = callbacks_executed;
}
