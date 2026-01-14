#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

// BIOS E820 Memory Map Entry
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) memory_map_entry_t;

// Memory types
#define MEMORY_AVAILABLE 1
#define MEMORY_RESERVED  2

// Block size for PMM (4KB)
#define BLOCK_SIZE 4096
#define BLOCKS_PER_BYTE 8

// Functions
void memory_init(void);

// Physical Memory Manager
void *pmm_alloc_block(void);
void pmm_free_block(void *ptr);

// Heap Allocator
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif
