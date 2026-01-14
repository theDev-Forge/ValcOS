#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef struct process {
    uint32_t *esp;       // Stack Pointer (Must be first for Assembly simplicity)
    uint32_t pid;        // Process ID
    uint32_t kernel_stack_top; // For TSS: where to restart kernel stack on interrupt
    uint32_t cr3;        // Page Directory Physical Address
    struct process *next;// Next process in linked list
} process_t;

// Global pointer to current process
extern process_t *current_process;

// Functions
void process_init(void);
void process_create(void (*entry_point)(void));
void process_create_user(void (*entry_point)(void));
void schedule(void);
void process_yield(void);

#endif
