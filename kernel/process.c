#include "process.h"
#include "memory.h"
#include "vga.h"
#include "tss.h"
#include "vmm.h"
#include "pmm.h"
#include "string.h"

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
    kernel_proc->cr3 = vmm_get_kernel_directory();
    kernel_proc->next = NULL;
    
    current_process = kernel_proc;
    ready_queue_head = kernel_proc;
    ready_queue_tail = kernel_proc;
}

void process_create(void (*entry_point)(void)) {
    // 1. Allocate PCB
    process_t *proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    proc->cr3 = vmm_get_kernel_directory();
    proc->next = NULL;
    
    // 2. Allocate Stack (4KB)
    uint32_t *stack = (uint32_t*)kmalloc(4096);
    uint32_t *top = stack + 1024; // End of stack (grows down)
    
    proc->kernel_stack_top = (uint32_t)top; // Save Top Address for TSS
    
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

extern void enter_user_mode(void);

void process_create_user(void (*entry_point)(void)) {
    // 1. Allocate PCB
    process_t *proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    proc->cr3 = vmm_get_kernel_directory();
    proc->next = NULL;

    // 2. Allocate Kernel Stack (4KB)
    uint32_t *kstack = (uint32_t*)kmalloc(4096);
    uint32_t *ktop = kstack + 1024;
    
    proc->kernel_stack_top = (uint32_t)ktop; // Save Top for TSS

    // 3. Allocate User Pages (Code & Stack)
    uint32_t phys_code = pmm_alloc_block();
    uint32_t phys_stack = pmm_alloc_block();
    
    // Map them at User Virtual Addresses with USER permissions
    // Code: 0x400000
    // Stack: 0x401000
    // Flags: 0x07 (Present | RW | User)
    vmm_map_page(phys_code, 0x400000, 0x07);
    vmm_map_page(phys_stack, 0x401000, 0x07);
    
    // Copy Code to 0x400000
    // entry_point is the source buffer address here
    memcpy((void*)0x400000, (void*)entry_point, 4096);

    // 4. Setup Trap Frame for IRET (User Mode Switch)
    // Stack: [SS] [ESP] [EFLAGS] [CS] [EIP]
    // Stack grows down from 0x402000 (0x401000 + 4096)
    uint32_t user_esp = 0x402000;

    *(--ktop) = 0x23;                   // User SS (Ring 3 Data)
    *(--ktop) = user_esp;               // User ESP
    *(--ktop) = 0x202;                  // User EFLAGS (IE=1)
    *(--ktop) = 0x1B;                   // User CS (Ring 3 Code)
    *(--ktop) = 0x400000;               // User EIP (Start of Code)

    // 5. Setup Stack Frame for switch_to_task
    // RET will jump to enter_user_mode (which does IRET)
    *(--ktop) = (uint32_t)enter_user_mode;

    // PUSHA/POPA Registers
    *(--ktop) = 0; // EAX
    *(--ktop) = 0; // ECX
    *(--ktop) = 0; // EDX
    *(--ktop) = 0; // EBX
    *(--ktop) = 0; // ESP
    *(--ktop) = 0; // EBP
    *(--ktop) = 0; // ESI
    *(--ktop) = 0; // EDI
    
    *(--ktop) = 0x202;                  // Kernel EFLAGS

    proc->esp = ktop;

    // 6. Add to Queue
    if (ready_queue_tail) {
        ready_queue_tail->next = proc;
        ready_queue_tail = proc;
        proc->next = ready_queue_head;
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
    
    if (next != current_process) {
        // Update TSS to use the new process's kernel stack for interrupts
        set_kernel_stack(next->kernel_stack_top);
        
        // Switch Page Directory
        if (next->cr3) {
            vmm_switch_directory(next->cr3);
        }
        
        switch_to_task(next);
    }
}

void process_yield(void) {
    schedule();
}
