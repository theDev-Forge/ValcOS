#include "slab.h"
#include "memory.h"
#include "pmm.h"
#include "vga.h"
#include "string.h"

// Global list of all caches
static LIST_HEAD(cache_list);

// Common size caches (for general kmalloc replacement)
kmem_cache_t *kmalloc_caches[8] = {NULL};

// Helper: Calculate number of objects that fit in a slab
static uint32_t calculate_objects_per_slab(size_t obj_size) {
    // Use one 4KB page per slab for simplicity
    size_t slab_size = PMM_BLOCK_SIZE;
    size_t usable_size = slab_size - sizeof(slab_t);
    return usable_size / obj_size;
}

// Helper: Allocate a new slab
static slab_t *slab_create(kmem_cache_t *cache) {
    // Allocate slab descriptor from general heap
    slab_t *slab = (slab_t*)kmalloc(sizeof(slab_t));
    if (!slab) return NULL;
    
    // Allocate memory for objects (one 4KB page)
    uint32_t phys_addr = pmm_alloc_block();
    if (!phys_addr) {
        kfree(slab);
        return NULL;
    }
    
    // For now, use physical address directly (identity mapping)
    // In a real system, you'd map this to virtual memory
    slab->mem = (void*)phys_addr;
    slab->inuse = 0;
    slab->objects = calculate_objects_per_slab(cache->object_size);
    
    // Initialize free list
    slab->freelist = slab->mem;
    slab_obj_t *obj = (slab_obj_t*)slab->mem;
    
    for (uint32_t i = 0; i < slab->objects - 1; i++) {
        slab_obj_t *next = (slab_obj_t*)((uint8_t*)obj + cache->object_size);
        obj->next = next;
        obj = next;
    }
    obj->next = NULL;  // Last object
    
    INIT_LIST_HEAD(&slab->list);
    cache->num_slabs++;
    
    return slab;
}

// Helper: Destroy a slab
static void slab_destroy(kmem_cache_t *cache, slab_t *slab) {
    if (!slab) return;
    
    // Free the memory page
    pmm_free_block((uint32_t)slab->mem);
    
    // Remove from list
    list_del(&slab->list);
    
    // Free slab descriptor
    kfree(slab);
    
    cache->num_slabs--;
}

void slab_init(void) {
    vga_print("Initializing Slab Allocator...\n");
    
    // Create common size caches for general allocation
    // These can replace kmalloc for common sizes
    const size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
    const char *names[] = {"kmalloc-32", "kmalloc-64", "kmalloc-128", "kmalloc-256",
                           "kmalloc-512", "kmalloc-1024", "kmalloc-2048", "kmalloc-4096"};
    
    for (int i = 0; i < 8; i++) {
        kmalloc_caches[i] = kmem_cache_create(names[i], sizes[i], 0, 0);
        if (!kmalloc_caches[i]) {
            vga_print("Warning: Failed to create ");
            vga_print(names[i]);
            vga_print(" cache\n");
        }
    }
    
    vga_print("Slab Allocator initialized.\n");
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size, 
                                size_t align, uint32_t flags) {
    // Allocate cache descriptor
    kmem_cache_t *cache = (kmem_cache_t*)kmalloc(sizeof(kmem_cache_t));
    if (!cache) return NULL;
    
    // Initialize cache
    strncpy(cache->name, name, 31);
    cache->name[31] = '\0';
    cache->object_size = size;
    cache->align = align ? align : sizeof(void*);  // Default alignment
    cache->flags = flags;
    
    // Align object size
    cache->object_size = (cache->object_size + cache->align - 1) & ~(cache->align - 1);
    
    // Ensure minimum size for free list pointer
    if (cache->object_size < sizeof(slab_obj_t)) {
        cache->object_size = sizeof(slab_obj_t);
    }
    
    // Initialize slab lists
    INIT_LIST_HEAD(&cache->slabs_full);
    INIT_LIST_HEAD(&cache->slabs_partial);
    INIT_LIST_HEAD(&cache->slabs_empty);
    
    // Initialize statistics
    cache->num_objs = 0;
    cache->num_active = 0;
    cache->num_slabs = 0;
    
    // Add to global cache list
    INIT_LIST_HEAD(&cache->list);
    list_add_tail(&cache->list, &cache_list);
    
    return cache;
}

void kmem_cache_destroy(kmem_cache_t *cache) {
    if (!cache) return;
    
    // Free all slabs
    slab_t *slab, *tmp;
    
    list_for_each_entry_safe(slab, tmp, &cache->slabs_empty, list) {
        slab_destroy(cache, slab);
    }
    
    list_for_each_entry_safe(slab, tmp, &cache->slabs_partial, list) {
        slab_destroy(cache, slab);
    }
    
    list_for_each_entry_safe(slab, tmp, &cache->slabs_full, list) {
        slab_destroy(cache, slab);
    }
    
    // Remove from global list
    list_del(&cache->list);
    
    // Free cache descriptor
    kfree(cache);
}

void *kmem_cache_alloc(kmem_cache_t *cache) {
    if (!cache) return NULL;
    
    slab_t *slab = NULL;
    
    // Try to allocate from partial slab first
    if (!list_empty(&cache->slabs_partial)) {
        slab = list_first_entry(&cache->slabs_partial, slab_t, list);
    }
    // Then try empty slab
    else if (!list_empty(&cache->slabs_empty)) {
        slab = list_first_entry(&cache->slabs_empty, slab_t, list);
    }
    // Create new slab if needed
    else {
        slab = slab_create(cache);
        if (!slab) return NULL;
        list_add(&slab->list, &cache->slabs_empty);
    }
    
    // Allocate object from slab
    if (!slab->freelist) return NULL;  // Should never happen
    
    slab_obj_t *obj = (slab_obj_t*)slab->freelist;
    slab->freelist = obj->next;
    slab->inuse++;
    
    // Move slab to appropriate list
    list_del(&slab->list);
    if (slab->inuse == slab->objects) {
        list_add(&slab->list, &cache->slabs_full);
    } else {
        list_add(&slab->list, &cache->slabs_partial);
    }
    
    // Update statistics
    cache->num_active++;
    cache->num_objs++;
    
    // Zero out the object (optional, but helpful)
    memset(obj, 0, cache->object_size);
    
    return (void*)obj;
}

void kmem_cache_free(kmem_cache_t *cache, void *obj) {
    if (!cache || !obj) return;
    
    // Find which slab this object belongs to
    // For simplicity, we'll search all slabs (inefficient but works)
    // A real implementation would use page-aligned slabs and calculate slab from address
    
    slab_t *slab = NULL;
    slab_t *pos;
    
    // Search in full slabs
    list_for_each_entry(pos, &cache->slabs_full, list) {
        if (obj >= pos->mem && obj < (void*)((uint8_t*)pos->mem + PMM_BLOCK_SIZE)) {
            slab = pos;
            break;
        }
    }
    
    // Search in partial slabs
    if (!slab) {
        list_for_each_entry(pos, &cache->slabs_partial, list) {
            if (obj >= pos->mem && obj < (void*)((uint8_t*)pos->mem + PMM_BLOCK_SIZE)) {
                slab = pos;
                break;
            }
        }
    }
    
    if (!slab) return;  // Object not found (error)
    
    // Add object back to free list
    slab_obj_t *free_obj = (slab_obj_t*)obj;
    free_obj->next = (slab_obj_t*)slab->freelist;
    slab->freelist = free_obj;
    slab->inuse--;
    
    // Move slab to appropriate list
    list_del(&slab->list);
    if (slab->inuse == 0) {
        list_add(&slab->list, &cache->slabs_empty);
    } else {
        list_add(&slab->list, &cache->slabs_partial);
    }
    
    // Update statistics
    cache->num_active--;
}

int kmem_cache_shrink(kmem_cache_t *cache) {
    if (!cache) return 0;
    
    int freed = 0;
    slab_t *slab, *tmp;
    
    // Free empty slabs (keep at least one)
    int empty_count = 0;
    list_for_each_entry(slab, &cache->slabs_empty, list) {
        empty_count++;
    }
    
    if (empty_count > 1) {
        list_for_each_entry_safe(slab, tmp, &cache->slabs_empty, list) {
            if (empty_count <= 1) break;  // Keep one empty slab
            slab_destroy(cache, slab);
            freed++;
            empty_count--;
        }
    }
    
    return freed;
}

void kmem_cache_info(kmem_cache_t *cache) {
    if (!cache) return;
    
    vga_print("Cache: ");
    vga_print(cache->name);
    vga_print("\n  Object size: ");
    
    char buf[16];
    int idx = 0;
    uint32_t n = cache->object_size;
    if(n==0) buf[idx++]='0'; 
    else { 
        char t[16]; 
        int j=0; 
        while(n>0){t[j++]='0'+(n%10);n/=10;} 
        while(j>0) buf[idx++]=t[--j]; 
    } 
    buf[idx]=0;
    vga_print(buf);
    
    vga_print(" bytes\n  Active objects: ");
    idx = 0; n = cache->num_active;
    if(n==0) buf[idx++]='0'; 
    else { 
        char t[16]; 
        int j=0; 
        while(n>0){t[j++]='0'+(n%10);n/=10;} 
        while(j>0) buf[idx++]=t[--j]; 
    } 
    buf[idx]=0;
    vga_print(buf);
    
    vga_print("\n  Total slabs: ");
    idx = 0; n = cache->num_slabs;
    if(n==0) buf[idx++]='0'; 
    else { 
        char t[16]; 
        int j=0; 
        while(n>0){t[j++]='0'+(n%10);n/=10;} 
        while(j>0) buf[idx++]=t[--j]; 
    } 
    buf[idx]=0;
    vga_print(buf);
    vga_print("\n");
}

void slab_stats(void) {
    vga_print("\n=== Slab Allocator Statistics ===\n");
    
    kmem_cache_t *cache;
    list_for_each_entry(cache, &cache_list, list) {
        kmem_cache_info(cache);
    }
    
    vga_print("\n");
}
