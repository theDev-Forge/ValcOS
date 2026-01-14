#include "pmm.h"
#include "string.h"
#include "vga.h"

// Bitmap size: 128MB RAM / 4KB blocks = 32768 blocks.
// 32768 bits / 8 = 4096 bytes.
#define BITMAP_SIZE 4096 

static uint8_t pmm_bitmap[BITMAP_SIZE];
static uint32_t total_blocks = 0;
static uint32_t used_blocks = 0;

void pmm_init(uint32_t mem_size) {
    vga_print("Initializing PMM...\n");
    
    total_blocks = mem_size / PMM_BLOCK_SIZE;
    used_blocks = total_blocks; // Initially mark all as used (safety) until we clear them?
    // Actually, usually we mark all as free, but we must protect Kernel memory.
    // Simpler: Mark ALL as free (0), then explicitly mark Kernel area as used.
    
    // Clear bitmap (0 = Free)
    memset(pmm_bitmap, 0, BITMAP_SIZE);
    used_blocks = 0;
    
    // Mark the first 4MB as used (Kernel + BIOS Area + VGA)
    // 4MB / 4KB = 1024 blocks.
    for (uint32_t i = 0; i < 1024; i++) {
        // Mark bit i as used (1)
        pmm_bitmap[i / 8] |= (1 << (i % 8));
        used_blocks++;
    }
}

uint32_t pmm_alloc_block(void) {
    // Find first free bit
    for (uint32_t i = 0; i < total_blocks; i++) {
        if (!((pmm_bitmap[i / 8] >> (i % 8)) & 1)) {
            // Found free (0)
            pmm_bitmap[i / 8] |= (1 << (i % 8)); // Mark as used
            used_blocks++;
            return i * PMM_BLOCK_SIZE;
        }
    }
    vga_print("PMM: Out of Memory!\n");
    return 0; // Return 0 on failure (Technically 0 is valid physical addr [IVT], but we marked it used)
}

void pmm_free_block(uint32_t addr) {
    uint32_t block = addr / PMM_BLOCK_SIZE;
    if (block < total_blocks) {
        if ((pmm_bitmap[block / 8] >> (block % 8)) & 1) {
            pmm_bitmap[block / 8] &= ~(1 << (block % 8)); // Clear bit
            used_blocks--;
        }
    }
}
