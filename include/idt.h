#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Registers struct (Pushed by PUSHA + CPU Interrupt)
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t eip, cs, eflags;                        // Pushed by CPU
    uint32_t user_esp, user_ss;                      // Pushed by CPU if switching rings
} __attribute__((packed)) registers_t;

// IDT entry structure
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Initialize the IDT
void idt_init(void);

// Set an IDT gate
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
