#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "vga.h"
#include "process.h"

// Page Directory Entry (PDE)
// Bit 0: Present
// Bit 1: RW
// Bit 2: User
// ...
// Bit 12-31: Page Table Address (Physical)

// Page Table Entry (PTE)
// Bit 12-31: Physical Frame Address

#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR  1024

// Kernel Page Directory (pointers to Page Tables)
// We will allocate this using PMM to ensure page alignment (4KB)
static uint32_t *kernel_directory = 0;

uint32_t vmm_get_kernel_directory(void) {
    return (uint32_t)kernel_directory;
}

void vmm_switch_directory(uint32_t dir_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(dir_phys));
}

void vmm_map_page(uint32_t phys, uint32_t virt, uint32_t flags) {
    // 1. Calculate Directory Index and Table Index
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x03FF;
    
    // 2. Check if Page Table exists
    // The PDE contains physical address of PT in top 20 bits
    uint32_t pde = kernel_directory[pd_index];
    
    uint32_t *table; // Virtual address of the table
    
    if (!(pde & PTE_PRESENT)) {
        // Table not present, allocate it
        uint32_t new_table_phys = pmm_alloc_block();
        if (!new_table_phys) {
            vga_print("VMM: Out of memory alloc table!\n");
            return;
        }
        
        // In our identity mapped Kernel, Phys = Virt for this stage
        table = (uint32_t*)new_table_phys;
        
        // Clear the new table
        memset((uint8_t*)table, 0, 4096);
        
        // Add Entry to Directory
        // Present | RW | User (if needed) -> Flags should include PTE_PRESENT | PTE_RW
        // Default table flags: present, rw
        kernel_directory[pd_index] = new_table_phys | PTE_PRESENT | PTE_RW | (flags & PTE_USER); 
    } else {
        // Table exists
        // If the new page requires USER permissions, we must ensure the PDE also has USER permissions.
        if (flags & PTE_USER) {
            kernel_directory[pd_index] |= PTE_USER;
        }
        
        // Extract Physical Address
        uint32_t table_phys = pde & 0xFFFFF000;
        // Assume Phys=Virt for now
        table = (uint32_t*)table_phys; 
    }
    
    // 3. Set Page Table Entry
    table[pt_index] = (phys & 0xFFFFF000) | flags;
    
    // 4. Invalidate TLB
    __asm__ volatile("invlpg (%0)" :: "r" (virt) : "memory");
}

void vmm_enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

void vmm_load_directory(uint32_t dir_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(dir_phys));
}

void vmm_init(void) {
    vga_print("Initializing VMM...\n");
    
    // 1. Allocate Kernel Directory
    uint32_t dir_phys = pmm_alloc_block();
    if (!dir_phys) {
        vga_print("VMM: Failed to alloc directory!\n");
        return;
    }
    
    kernel_directory = (uint32_t*)dir_phys;
    memset((uint8_t*)kernel_directory, 0, 4096);
    
    // 2. Identity Map Kernel (0 - 4MB)
    // Actually, let's map 0 - 8MB to be safe for Heap growth
    // 0xB8000 (VGA) is inside this range.
    // 0x100000 (Kernel Start) is inside.
    
    // Map 0 -> 0 to 8MB -> 8MB
    for (uint32_t addr = 0; addr < 0x800000; addr += PAGE_SIZE) {
        vmm_map_page(addr, addr, PTE_PRESENT | PTE_RW); // Kernel pages
    }
    
    // 3. Load CR3
    vmm_load_directory(dir_phys);
    
    // 4. Enable Paging
    vmm_enable_paging();
    
    vga_print("Paging Enabled!\n");
}

void page_fault_handler(uint32_t err_code) {
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r" (fault_addr));
    
    vga_print("\n[PAGE FAULT] ");
    
    const char *digits = "0123456789ABCDEF";
    
    vga_print("Addr: 0x");
    for (int i = 28; i >= 0; i -= 4) {
        char c = digits[(fault_addr >> i) & 0xF];
        char str[2] = {c, '\0'};
        vga_print(str);
    }
    
    vga_print(" Err: 0x");
    for (int i = 28; i >= 0; i -= 4) {
        char c = digits[(err_code >> i) & 0xF];
        char str[2] = {c, '\0'};
        vga_print(str);
    }
    
    vga_print("\nFlags: ");
    if (err_code & 1) vga_print("[P] "); else vga_print("[NP] ");
    if (err_code & 2) vga_print("[W] "); else vga_print("[R] ");
    if (err_code & 4) vga_print("[U] "); else vga_print("[K] ");
    
    vga_print("\nSystem Halted.\n");
    
    while(1) __asm__ volatile("hlt");
}

void gpf_handler(void) {
    vga_print_color("\n[GPF] General Protection Fault!\n", 0x0C);
    vga_print("System Halted.\n");
    while(1) __asm__ volatile("hlt");
}

void double_fault_handler(void) {
    vga_print_color("\n[DOUBLE FAULT] System Panic!\n", 0x0C);
    vga_print("System Halted.\n");
    while(1) __asm__ volatile("hlt");
}
