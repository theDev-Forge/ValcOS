#include "tss.h"
#include "gdt.h"
#include "string.h"
#include "vga.h"

static tss_entry_t tss_entry;

void tss_flush(void); // Defined in ASM (we can also do inline asm)

void init_tss(void) {
    vga_print("Initializing TSS...\n");
    
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry);

    // Add TSS descriptor to GDT (Entry 5)
    // Access: 0xE9 = Present | Ring 3 | Executable | Accessed (TSS Type 9)
    // Wait, Type 9 (32-bit TSS Available) is correct.
    // DPL=0 generally for TSS.
    // 0x89 = P=1, DPL=0, Type=9.
    // Let's use 0xE9? No, TSS itself is not user accessible usually. 0x89 is safer.
    // But let's check standard. 
    // GDT Access Byte:
    // P(1) DPL(2) S(1) Type(4)
    // S=0 for System Segment (TSS/LDT/Gate).
    // Type=1001 (0x9) for 32-bit TSS Available.
    // P=1.
    // DPL=0 (User doesn't switch task manually).
    // So 1 00 0 1001 = 0x89.
    
    gdt_set_gate(5, base, limit, 0x89, 0x00); // 0x00 granularity (byte granular)

    // Zero out TSS
    memset((uint8_t*)&tss_entry, 0, sizeof(tss_entry));

    // Set Kernel Stack Segment
    tss_entry.ss0 = 0x10;  // Kernel Data Segment
    
    // Set initial Kernel Stack (will be updated by scheduler)
    // tss_entry.esp0 = 0; 
    
    // Load TR
    // 0x28 = Index 5 * 8 = 40. Or with RPL=0/3? 
    // TR is loaded with a selector. RPL should be 0.
    // LTR instruction.
    __asm__ volatile("ltr %%ax" : : "a" (0x28));
}

void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}
