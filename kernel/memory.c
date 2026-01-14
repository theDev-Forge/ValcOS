#include "memory.h"
#include "vga.h"
#include "string.h"

// Memory map location (from bootloader)
#define MEMORY_MAP_COUNT_ADDR 0x7000
#define MEMORY_MAP_ADDR       0x7004

// Bitmap for PMM
// 1024 * 1024 * 1024 bits = 128 MB managed (32768 blocks)
// Let's manage 128MB for now. 128MB / 4KB = 32768 blocks.
// Bitmap size = 32768 / 8 = 4096 bytes.
#define BITMAP_SIZE 4096
static uint8_t memory_bitmap[BITMAP_SIZE];

// Heap
// Let's place the heap at 2MB mark (0x200000) for now
// and give it 1MB size.
#define HEAP_START 0x200000
#define HEAP_SIZE  0x100000

typedef struct heap_block {
    size_t size;
    uint8_t is_free;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *heap_head = (heap_block_t *)HEAP_START;

void pmm_init(void) {
    uint16_t entry_count = *(uint16_t*)MEMORY_MAP_COUNT_ADDR;
    memory_map_entry_t *map = (memory_map_entry_t*)MEMORY_MAP_ADDR;
    
    // Default: Mark all memory as used
    memset(memory_bitmap, 0xFF, BITMAP_SIZE);
    
    // vga_print("Memory Map:\n");
    
    for (int i = 0; i < entry_count; i++) {
        // Only use available memory
        if (map[i].type == MEMORY_AVAILABLE) {
            uint64_t base = map[i].base;
            uint64_t length = map[i].length;
            
            // Limit to our managed range (0 - 128MB)
            if (base < 128 * 1024 * 1024) {
                 // Mark bits as free (0)
                 int start_block = base / BLOCK_SIZE;
                 int num_blocks = length / BLOCK_SIZE;
                 
                 for (int j = 0; j < num_blocks; j++) {
                     int block = start_block + j;
                     if (block < BITMAP_SIZE * 8) {
                         // Clear bit (free)
                         memory_bitmap[block / 8] &= ~(1 << (block % 8));
                     }
                 }
            }
        }
    }
    
    // Reserve specific regions
    // 0 - 1MB (Kernel, Code, etc.) - Mark as used (1)
    for (int i = 0; i < 256; i++) { // 256 blocks * 4KB = 1MB
         memory_bitmap[i / 8] |= (1 << (i % 8));
    }
    
    // Reserve Heap Region (1MB at 2MB mark)
    int heap_start_block = HEAP_START / BLOCK_SIZE;
    int heap_blocks = HEAP_SIZE / BLOCK_SIZE;
    for (int i = 0; i < heap_blocks; i++) {
         int block = heap_start_block + i;
         memory_bitmap[block / 8] |= (1 << (block % 8));
    }
}

void *pmm_alloc_block(void) {
    for (int i = 0; i < BITMAP_SIZE * 8; i++) {
        if (!(memory_bitmap[i / 8] & (1 << (i % 8)))) {
            // Found free block
            memory_bitmap[i / 8] |= (1 << (i % 8)); // Mark as used
            return (void*)(i * BLOCK_SIZE);
        }
    }
    return NULL; // Out of memory
}

void pmm_free_block(void *ptr) {
    int block = (uint32_t)ptr / BLOCK_SIZE;
    memory_bitmap[block / 8] &= ~(1 << (block % 8)); // Mark as free
}

void heap_init(void) {
    heap_head->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
}

void *kmalloc(size_t size) {
    heap_block_t *current = heap_head;
    
    // Align size to 16 bytes for safety
    size = (size + 15) & ~15;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            // Found a block
            
            // Check if we can split it
            if (current->size > size + sizeof(heap_block_t) + 16) {
                heap_block_t *new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    heap_block_t *block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->is_free = 1;
    
    // Merge with next block if free
    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
    }
}

void memory_init(void) {
    pmm_init();
    heap_init();
    
    vga_print("Memory initialized.\n");
}
