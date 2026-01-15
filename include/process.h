#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "list.h"

// Process states
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process {
    uint32_t esp;        // Stack Pointer (Must be first for Assembly simplicity)
    uint32_t pid;        // Process ID
    uint32_t kernel_stack_top; // For TSS: where to restart kernel stack on interrupt
    uint32_t cr3;        // Page Directory Physical Address
    struct list_head list;     // Linux-style linked list node
    
    // Preemptive multitasking fields
    process_state_t state;      // Current process state
    uint8_t priority;           // 0-255 (higher = more priority)
    uint32_t time_slice;        // Remaining ticks in current slice
    uint32_t total_runtime;     // Total ticks executed
    char name[32];              // Process name for debugging
} process_t;

// Global pointer to current process
extern process_t *current_process;

// Functions
void process_init(void);
void process_create(void (*entry_point)(void));
void process_create_user(void (*entry_point)(void));

// Debug: List all processes
void process_debug_list(void);
// Kill a process by PID
int process_kill(uint32_t pid);
void schedule(void);
void process_yield(void);

// New process management functions
void process_set_priority(uint32_t pid, uint8_t priority);
void process_block(uint32_t pid);
void process_unblock(uint32_t pid);
void process_get_stats(uint32_t pid, uint32_t *runtime, uint8_t *priority, process_state_t *state);

#endif
