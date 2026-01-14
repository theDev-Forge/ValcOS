#include "process.h"
#include "memory.h"
#include "vga.h"

process_t *current_process = NULL;
process_t *ready_queue_head = NULL;
process_t *ready_queue_tail = NULL;
uint32_t next_pid = 1;

extern void switch_to_task(process_t *next);

void process_init(void) {
    vga_print("Initializing Multitasking...\n");
    
    // Create struct for the currently running kernel task
    // We don't need to weirdly allocate a stack for it, it HAS a stack.
    // We just need a PCB to save its state into when we switch OUT of it.
    process_t *kernel_proc = (process_t*)kmalloc(sizeof(process_t));
    kernel_proc->pid = 0;
    kernel_proc->esp = 0; // Will be set by switch_to_task
    kernel_proc->next = NULL;
    
    current_process = kernel_proc;
    ready_queue_head = kernel_proc;
    ready_queue_tail = kernel_proc;
}

void process_create(void (*entry_point)(void)) {
    // 1. Allocate PCB
    process_t *proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    proc->next = NULL;
    
    // 2. Allocate Stack (4KB)
    uint32_t *stack = (uint32_t*)kmalloc(4096);
    uint32_t *top = stack + 1024; // End of stack (grows down)
    
    // 3. Setup Stack Frame (simulating 'pusha' and 'pushf')
    // Values popped by switch_to_task:
    // POPF (EFLAGS)
    // POPA (EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX)
    // RET (EIP)
    
    *(--top) = (uint32_t)entry_point;   // Return Address (EIP)
    
    // Registers (matching PUSHA/POPA)
    // Stack direction is Down.
    // High Addr -> [EIP]
    //              [EAX]
    //              [ECX] 
    //              ...
    //              [EDI]
    // Low Addr  -> [EFLAGS]
    
    *(--top) = 0; // EAX
    *(--top) = 0; // ECX
    *(--top) = 0; // EDX
    *(--top) = 0; // EBX
    *(--top) = 0; // ESP (Ignored)
    *(--top) = 0; // EBP
    *(--top) = 0; // ESI
    *(--top) = 0; // EDI
    
    *(--top) = 0x202;                   // EFLAGS (Overwrites top, Interrupts Enabled)
    
    proc->esp = top;
    
    // 4. Add to Queue
    if (ready_queue_tail) {
        ready_queue_tail->next = proc;
        ready_queue_tail = proc;
        proc->next = ready_queue_head; // Circular list for Round Robin
    } else {
        ready_queue_head = proc;
        ready_queue_tail = proc;
        proc->next = proc;
    }
}

void schedule(void) {
    if (!current_process) return;
    
    process_t *next = current_process->next;
    if (!next) return;
    // vga_print("S"); // Debug: Switching
    
    if (next != current_process) {
        switch_to_task(next);
    }
}

void process_yield(void) {
    schedule();
}
