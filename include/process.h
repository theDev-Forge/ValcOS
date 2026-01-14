#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef struct process {
    uint32_t *esp;       // Stack Pointer (Must be first for Assembly simplicity)
    uint32_t pid;        // Process ID
    struct process *next;// Next process in linked list
} process_t;

// Global pointer to current process
extern process_t *current_process;

// Functions
void process_init(void);
void process_create(void (*entry_point)(void));
void schedule(void);
void process_yield(void);

#endif
