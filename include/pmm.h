#ifndef PMM_H
#define PMM_H

#include <stdint.h>

// 4KB Block Size
#define PMM_BLOCK_SIZE 4096

// Memory region structure
typedef struct {
    uint32_t base;
    uint32_t length;
    uint32_t type; // 1 = usable, 2 = reserved
} memory_region_t;

// Functions
void pmm_init(uint32_t mem_size);

// Single block allocation
uint32_t pmm_alloc_block(void);
void pmm_free_block(uint32_t addr);

// Contiguous block allocation
uint32_t pmm_alloc_blocks(uint32_t count);
void pmm_free_blocks(uint32_t addr, uint32_t count);

// Reserve specific memory region
void pmm_reserve_region(uint32_t start, uint32_t size);

// Get Memory Statistics
void pmm_get_stats(uint32_t *total, uint32_t *used);
uint32_t pmm_get_free_memory(void);
uint32_t pmm_get_total_memory(void);

#endif
