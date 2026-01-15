#include "pmm.h"
#include "printk.h"
#include "string.h"

// Bitmap size calculation
// 4GB RAM / 4KB Blocks = 1,048,576 Blocks
// Bitmap needs 1 bit per block -> 1,048,576 bits
// 1,048,576 / 32 bits per uint32_t = 32,768 uint32_t entries
#define MAX_BLOCKS 1048576
#define BITMAP_SIZE (MAX_BLOCKS / 32)

static uint32_t pmm_bitmap[BITMAP_SIZE];
static uint32_t total_blocks = 0;
static uint32_t used_blocks = 0;

// Helper: Set bit in bitmap
static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

// Helper: Clear bit in bitmap
static inline void bitmap_unset(uint32_t bit) {
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

// Helper: Test bit in bitmap
static inline int bitmap_test(uint32_t bit) {
    return pmm_bitmap[bit / 32] & (1 << (bit % 32));
}

// Helper: Find first free bit (optimized with 32-bit scanning)
static uint32_t bitmap_find_free(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            // This uint32_t has at least one free bit
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t bit = i * 32 + j;
                if (bit >= total_blocks) return 0xFFFFFFFF;
                if (!bitmap_test(bit)) {
                    return bit;
                }
            }
        }
    }
    return 0xFFFFFFFF; // No free blocks
}

// Helper: Find contiguous free blocks
static uint32_t bitmap_find_free_contiguous(uint32_t count) {
    uint32_t found = 0;
    uint32_t start = 0;
    
    for (uint32_t i = 0; i < total_blocks; i++) {
        if (!bitmap_test(i)) {
            if (found == 0) start = i;
            found++;
            if (found == count) return start;
        } else {
            found = 0;
        }
    }
    
    return 0xFFFFFFFF; // Not enough contiguous blocks
}

void pmm_init(uint32_t mem_size) {
    pr_info("Initializing PMM...\n");
    
    total_blocks = mem_size / PMM_BLOCK_SIZE;
    if (total_blocks > MAX_BLOCKS) {
        total_blocks = MAX_BLOCKS;
    }
    
    used_blocks = 0;
    
    // Clear bitmap (0 = Free) - using optimized memset
    memset((uint8_t*)pmm_bitmap, 0, BITMAP_SIZE * sizeof(uint32_t));
    
    // Mark the first 4MB as used (Kernel + BIOS Area + VGA)
    // 4MB / 4KB = 1024 blocks.
    for (uint32_t i = 0; i < 1024; i++) {
        bitmap_set(i);
        used_blocks++;
    }
    
    // Display memory info
    uint32_t total_mb = (total_blocks * PMM_BLOCK_SIZE) / (1024 * 1024);
    pr_info("PMM: Total Memory: %d MB\n", total_mb);
}

uint32_t pmm_alloc_block(void) {
    uint32_t block = bitmap_find_free();
    
    if (block == 0xFFFFFFFF) {
        pr_err("PMM: Out of Memory!\n");
        return 0;
    }
    
    bitmap_set(block);
    used_blocks++;
    return block * PMM_BLOCK_SIZE;
}

void pmm_free_block(uint32_t addr) {
    uint32_t block = addr / PMM_BLOCK_SIZE;
    
    if (block < total_blocks && bitmap_test(block)) {
        bitmap_unset(block);  // Corrected function name
        used_blocks--;
    }
}

uint32_t pmm_alloc_blocks(uint32_t count) {
    if (count == 0) return 0;
    if (count == 1) return pmm_alloc_block();
    
    uint32_t start_block = bitmap_find_free_contiguous(count);
    
    if (start_block == 0xFFFFFFFF) {
        pr_err("PMM: Cannot allocate contiguous blocks!\n");
        return 0;
    }
    
    // Mark all blocks as used
    for (uint32_t i = 0; i < count; i++) {
        bitmap_set(start_block + i);
        used_blocks++;
    }
    
    return start_block * PMM_BLOCK_SIZE;
}

void pmm_free_blocks(uint32_t addr, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pmm_free_block(addr + (i * PMM_BLOCK_SIZE));
    }
}

void pmm_reserve_region(uint32_t start, uint32_t size) {
    uint32_t start_block = start / PMM_BLOCK_SIZE;
    uint32_t block_count = (size + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
    
    for (uint32_t i = 0; i < block_count; i++) {
        uint32_t block = start_block + i;
        if (block < total_blocks && !bitmap_test(block)) {
            bitmap_set(block);
            used_blocks++;
        }
    }
}

void pmm_get_stats(uint32_t *total, uint32_t *used) {
    if (total) *total = total_blocks;
    if (used) *used = used_blocks;
}

uint32_t pmm_get_free_memory(void) {
    return (total_blocks - used_blocks) * PMM_BLOCK_SIZE;
}

uint32_t pmm_get_total_memory(void) {
    return total_blocks * PMM_BLOCK_SIZE;
}
