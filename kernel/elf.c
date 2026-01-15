#include "elf.h"
#include "memory.h"
#include "string.h"
#include "printk.h"
#include "process.h"
#include "vmm.h"
#include "pmm.h"

// User stack location (grows down from 3GB)
#define USER_STACK_TOP 0xC0000000
#define USER_STACK_SIZE (4096 * 4)  // 16KB stack

int elf_validate(elf32_ehdr_t *ehdr) {
    if (!ehdr) return -1;
    
    // Check magic number
    if (ehdr->e_ident[0] != 0x7F ||
        ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' ||
        ehdr->e_ident[3] != 'F') {
        pr_err("ELF: Invalid magic number\n");
        return -1;
    }
    
    // Check class (32-bit)
    if (ehdr->e_ident[4] != ELFCLASS32) {
        pr_err("ELF: Not 32-bit\n");
        return -1;
    }
    
    // Check endianness (little endian)
    if (ehdr->e_ident[5] != ELFDATA2LSB) {
        pr_err("ELF: Not little endian\n");
        return -1;
    }
    
    // Check type (executable)
    if (ehdr->e_type != ET_EXEC) {
        pr_err("ELF: Not executable\n");
        return -1;
    }
    
    // Check machine (x86)
    if (ehdr->e_machine != EM_386) {
        pr_err("ELF: Not x86\n");
        return -1;
    }
    
    pr_debug("ELF: Valid 32-bit x86 executable\n");
    return 0;
}

uint32_t elf_load(void *data, uint32_t size) {
    if (!data || size < sizeof(elf32_ehdr_t)) {
        pr_err("ELF: Invalid data\n");
        return 0;
    }
    
    elf32_ehdr_t *ehdr = (elf32_ehdr_t *)data;
    
    // Validate ELF header
    if (elf_validate(ehdr) < 0) {
        return 0;
    }
    
    pr_info("ELF: Loading program (entry: 0x%x)\n", ehdr->e_entry);
    
    // Get program headers
    elf32_phdr_t *phdr = (elf32_phdr_t *)((uint8_t *)data + ehdr->e_phoff);
    
    // Load each PT_LOAD segment
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            pr_debug("ELF: Loading segment %d at 0x%x (size: %d bytes)\n",
                    i, phdr[i].p_vaddr, phdr[i].p_memsz);
            
            // Allocate pages for this segment
            uint32_t pages_needed = (phdr[i].p_memsz + 4095) / 4096;
            
            for (uint32_t p = 0; p < pages_needed; p++) {
                uint32_t vaddr = phdr[i].p_vaddr + (p * 4096);
                uint32_t paddr = pmm_alloc_block();
                
                if (paddr == 0) {
                    pr_err("ELF: Failed to allocate memory\n");
                    return 0;
                }
                
                // Map page (user accessible, writable)
                // Flags: PRESENT | RW | USER
                vmm_map_page(paddr, vaddr, PTE_PRESENT | PTE_RW | PTE_USER);
            }
            
            // Copy segment data
            if (phdr[i].p_filesz > 0) {
                memcpy((void *)phdr[i].p_vaddr, 
                       (uint8_t *)data + phdr[i].p_offset,
                       phdr[i].p_filesz);
            }
            
            // Zero BSS (uninitialized data)
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                memset((uint8_t *)phdr[i].p_vaddr + phdr[i].p_filesz,
                       0,
                       phdr[i].p_memsz - phdr[i].p_filesz);
            }
        }
    }
    
    pr_info("ELF: Program loaded successfully\n");
    return ehdr->e_entry;
}

// Jump to user mode and execute
extern void enter_usermode(uint32_t entry, uint32_t stack);

int elf_exec(const char *path) {
    if (!path) return -1;
    
    pr_info("ELF: Executing %s\n", path);
    
    // For now, we'll use a hardcoded test
    // In a full implementation, this would:
    // 1. Open file from VFS
    // 2. Read into buffer
    // 3. Call elf_load()
    
    // Allocate user stack
    uint32_t stack_pages = USER_STACK_SIZE / 4096;
    uint32_t stack_bottom = USER_STACK_TOP - USER_STACK_SIZE;
    
    for (uint32_t i = 0; i < stack_pages; i++) {
        uint32_t vaddr = stack_bottom + (i * 4096);
        uint32_t paddr = pmm_alloc_block();
        
        if (paddr == 0) {
            pr_err("ELF: Failed to allocate stack\n");
            return -1;
        }
        
        // Map page (user accessible, writable)
        vmm_map_page(paddr, vaddr, PTE_PRESENT | PTE_RW | PTE_USER);
    }
    
    pr_info("ELF: User stack allocated at 0x%x\n", USER_STACK_TOP);
    
    // For demonstration, we'll just print a message
    // Full implementation would call enter_usermode()
    pr_info("ELF: Ready to execute (user mode switching not fully implemented)\n");
    pr_info("ELF: Entry point: 0x%x, Stack: 0x%x\n", 0x400000, USER_STACK_TOP);
    
    return 0;
}

