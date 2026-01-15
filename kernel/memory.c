#include "memory.h"
#include "printk.h"
#include "string.h"

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
    heap_init();
    
    pr_info("Memory initialized.\n");
}
