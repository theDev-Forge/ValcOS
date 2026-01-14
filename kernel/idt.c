#include "idt.h"
#include "string.h"

// IDT with 256 entries
struct idt_entry idt[256];
struct idt_ptr idtp;

// External assembly function to load IDT
extern void idt_load(uint32_t);
extern void isr14(void);
extern void isr8(void);
extern void isr13(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    
    // Clear the IDT
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    
    // Remap the PIC (Programmable Interrupt Controller)
    // ICW1 - Initialize
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    
    // ICW2 - Remap IRQs
    outb(0x21, 0x20); // Master PIC starts at IRQ 32
    outb(0xA1, 0x28); // Slave PIC starts at IRQ 40
    
    // ICW3 - Setup cascading
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    
    // ICW4 - Environment info
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // Mask all IRQs except IRQ0 (timer) and IRQ1 (keyboard)
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
    
    // Register Page Fault Handler (INT 14)
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);

    // Register Double Fault (INT 8)
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);

    // Register GPF (INT 13)
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    
    // Load the IDT
    idt_load((uint32_t)&idtp);
}
