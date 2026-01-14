#include "gdt.h"
#include "vga.h"

// 6 GDT entries: Null, Kernel Code, Kernel Data, User Code, User Data, TSS
#define GDT_ENTRIES 6

gdt_entry_t gdt_entries[GDT_ENTRIES];
gdt_ptr_t   gdt_ptr;

extern void gdt_flush(uint32_t);

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= (gran & 0xF0);
    gdt_entries[num].access      = access;
}

void init_gdt(void) {
    vga_print("Initializing GDT...\n");

    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 0: Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: Kernel Code Segment (0x08)
    // Base=0, Limit=4GB, Access=0x9A (Present, Ring0, Code, Readable), Gran=0xCF (4KB blocks, 32-bit)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 2: Kernel Data Segment (0x10)
    // Access=0x92 (Present, Ring0, Data, Writable)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 3: User Code Segment (0x18)
    // Access=0xFA (Present, Ring3, Code, Readable)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4: User Data Segment (0x20)
    // Access=0xF2 (Present, Ring3, Data, Writable)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // 5: TSS (Will be set up later by tss.c, but reserve it)
    gdt_set_gate(5, 0, 0, 0, 0);

    gdt_flush((uint32_t)&gdt_ptr);
}
