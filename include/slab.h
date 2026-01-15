#ifndef SLAB_H
#define SLAB_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"

/**
 * Linux-Style Slab Allocator
 * 
 * Provides object caching for frequently allocated/freed kernel objects.
 * Benefits:
 * - Reduces fragmentation
 * - Faster allocation/deallocation
 * - Cache-friendly (objects pre-initialized)
 * - Reduces overhead of general-purpose allocator
 * 
 * Simplified from Linux SLUB allocator (no per-CPU caches, no NUMA)
 */

// Slab flags
#define SLAB_HWCACHE_ALIGN  0x00002000  // Align objects to cache lines
#define SLAB_PANIC          0x00040000  // Panic if allocation fails

// Forward declarations
struct kmem_cache;
struct slab;

/**
 * kmem_cache - Cache descriptor for a specific object type
 * 
 * Each cache manages objects of a single size.
 * Example: process_cache for process_t structures
 */
typedef struct kmem_cache {
    char name[32];              // Cache name for debugging
    size_t object_size;         // Size of each object
    size_t align;               // Alignment requirement
    uint32_t flags;             // Cache flags
    
    // Slab lists
    struct list_head slabs_full;    // Slabs with no free objects
    struct list_head slabs_partial; // Slabs with some free objects
    struct list_head slabs_empty;   // Slabs with all objects free
    
    // Statistics
    uint32_t num_objs;          // Total objects allocated
    uint32_t num_active;        // Active (in-use) objects
    uint32_t num_slabs;         // Total number of slabs
    
    // Cache list (global list of all caches)
    struct list_head list;
} kmem_cache_t;

/**
 * slab - A slab containing multiple objects
 * 
 * Each slab is one or more pages containing objects of the cache's size.
 */
typedef struct slab {
    struct list_head list;      // Link in cache's slab list
    void *mem;                  // Pointer to slab memory
    uint32_t inuse;             // Number of allocated objects
    uint32_t objects;           // Total objects in this slab
    void *freelist;             // Pointer to first free object
} slab_t;

/**
 * Object header for free list management
 * This is stored at the beginning of each free object
 */
typedef struct slab_obj {
    struct slab_obj *next;      // Next free object
} slab_obj_t;

// Slab allocator API

/**
 * kmem_cache_create - Create a new object cache
 * @name: Name of the cache (for debugging)
 * @size: Size of each object
 * @align: Alignment requirement (0 for default)
 * @flags: Cache flags (SLAB_HWCACHE_ALIGN, etc.)
 * 
 * Returns: Pointer to cache descriptor, or NULL on failure
 */
kmem_cache_t *kmem_cache_create(const char *name, size_t size, 
                                size_t align, uint32_t flags);

/**
 * kmem_cache_destroy - Destroy a cache and free all slabs
 * @cache: Cache to destroy
 * 
 * Note: All objects must be freed before destroying the cache
 */
void kmem_cache_destroy(kmem_cache_t *cache);

/**
 * kmem_cache_alloc - Allocate an object from the cache
 * @cache: Cache to allocate from
 * 
 * Returns: Pointer to allocated object, or NULL on failure
 */
void *kmem_cache_alloc(kmem_cache_t *cache);

/**
 * kmem_cache_free - Free an object back to the cache
 * @cache: Cache the object belongs to
 * @obj: Object to free
 */
void kmem_cache_free(kmem_cache_t *cache, void *obj);

/**
 * kmem_cache_shrink - Free empty slabs to reduce memory usage
 * @cache: Cache to shrink
 * 
 * Returns: Number of slabs freed
 */
int kmem_cache_shrink(kmem_cache_t *cache);

/**
 * kmem_cache_info - Print cache statistics
 * @cache: Cache to display info for
 */
void kmem_cache_info(kmem_cache_t *cache);

/**
 * slab_init - Initialize the slab allocator subsystem
 * 
 * Must be called before using any slab functions
 */
void slab_init(void);

/**
 * slab_stats - Display statistics for all caches
 */
void slab_stats(void);

// Common cache sizes (can be created on demand)
extern kmem_cache_t *kmalloc_caches[8];  // 32, 64, 128, 256, 512, 1024, 2048, 4096

#endif /* SLAB_H */
