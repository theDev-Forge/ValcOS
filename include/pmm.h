#ifndef PMM_H
#define PMM_H

#include <stdint.h>

// 4KB Block Size
#define PMM_BLOCK_SIZE 4096

// Functions
void pmm_init(uint32_t mem_size);
uint32_t pmm_alloc_block(void);
void pmm_free_block(uint32_t addr);

// Get Memory Statistics
void pmm_get_stats(uint32_t *total, uint32_t *used);

#endif
