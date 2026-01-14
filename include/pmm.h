#ifndef PMM_H
#define PMM_H

#include <stdint.h>

// 4KB Block Size
#define PMM_BLOCK_SIZE 4096

// Functions
void pmm_init(uint32_t mem_size);
uint32_t pmm_alloc_block(void);
void pmm_free_block(uint32_t addr);

#endif
