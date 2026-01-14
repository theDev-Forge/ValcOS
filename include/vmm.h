#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Page Table Entry Flags
#define PTE_PRESENT 0x1
#define PTE_RW      0x2
#define PTE_USER    0x4

// Size of one Page
#define PAGE_SIZE 4096

// Functions
void vmm_init(void);

// Maps a physical page to a virtual address
void vmm_map_page(uint32_t phys, uint32_t virt, uint32_t flags);

// Switch Page Directory (used for Context Switching later)
void vmm_switch_directory(uint32_t dir_phys);
uint32_t vmm_get_kernel_directory(void);

#endif
