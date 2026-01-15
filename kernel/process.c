#include "process.h"
#include "memory.h"
#include "slab.h"
#include "printk.h"
#include "tss.h"
#include "vmm.h"
#include "pmm.h"
#include "string.h"

process_t *current_process = NULL;
LIST_HEAD(ready_queue);  // Linux-style list head for ready queue
uint32_t next_pid = 1;

// Slab cache for process structures
static kmem_cache_t *process_cache = NULL;

// Scheduler configuration
#define DEFAULT_TIME_SLICE 10
#define DEFAULT_PRIORITY 128

// Calculate time slice based on priority
static uint32_t calculate_time_slice(uint8_t priority) {
    // Priority 0-127: 5-10 ticks
    // Priority 128-255: 10-20 ticks
    return DEFAULT_TIME_SLICE + (priority / 64);
}

extern void switch_to_task(process_t *next);

void process_init(void) {
    pr_info("Initializing Multitasking...\n");
    
    // Create slab cache for process structures
    process_cache = kmem_cache_create("process_cache", sizeof(process_t), 0, 0);
    if (!process_cache) {
        pr_err("Failed to create process cache!\n");
        return;
    }
    
    // Allocate kernel process from cache
    process_t *kernel_proc = (process_t*)kmem_cache_alloc(process_cache);
    kernel_proc->pid = 0;
    kernel_proc->esp = 0;
    kernel_proc->cr3 = vmm_get_kernel_directory();
    
    uint32_t current_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(current_esp));
    kernel_proc->kernel_stack_top = current_esp;
    
    // Initialize new fields
    kernel_proc->state = PROCESS_RUNNING;
    kernel_proc->priority = 255; // Kernel has highest priority
    kernel_proc->time_slice = calculate_time_slice(255);
    kernel_proc->total_runtime = 0;
    strcpy(kernel_proc->name, "kernel");
    
    // Initialize list node and add to ready queue
    INIT_LIST_HEAD(&kernel_proc->list);
    list_add_tail(&kernel_proc->list, &ready_queue);
    
    current_process = kernel_proc;
}

void process_create(void (*entry_point)(void)) {
    // Allocate process from slab cache
    process_t *proc = (process_t*)kmem_cache_alloc(process_cache);
    proc->pid = next_pid++;
    proc->cr3 = vmm_get_kernel_directory();
    
    // Initialize new fields
    proc->state = PROCESS_READY;
    proc->priority = DEFAULT_PRIORITY;
    proc->time_slice = calculate_time_slice(DEFAULT_PRIORITY);
    proc->total_runtime = 0;
    strcpy(proc->name, "process");
    
    uint32_t *stack = (uint32_t*)kmalloc(4096);
    uint32_t *top = stack + 1024;
    
    proc->kernel_stack_top = (uint32_t)top;
    
    *(--top) = (uint32_t)entry_point;
    *(--top) = 0; // EAX
    *(--top) = 0; // ECX
    *(--top) = 0; // EDX
    *(--top) = 0; // EBX
    *(--top) = 0; // ESP
    *(--top) = 0; // EBP
    *(--top) = 0; // ESI
    *(--top) = 0; // EDI
    *(--top) = 0x202; // EFLAGS
    
    proc->esp = (uint32_t)top;
    
    // Add to ready queue using Linux-style list
    INIT_LIST_HEAD(&proc->list);
    list_add_tail(&proc->list, &ready_queue);
}

extern void enter_user_mode(void);

void process_create_user(void (*entry_point)(void)) {
    // 1. Allocate PCB from slab cache
    process_t *proc = (process_t*)kmem_cache_alloc(process_cache);
    proc->pid = next_pid++;
    proc->cr3 = vmm_get_kernel_directory();

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

    proc->esp = (uint32_t)ktop;

    // 6. Add to Queue using Linux-style list
    INIT_LIST_HEAD(&proc->list);
    list_add_tail(&proc->list, &ready_queue);
}

void schedule(void) {
    if (!current_process) return;
    
    // Decrement time slice for current process
    if (current_process->time_slice > 0) {
        current_process->time_slice--;
        current_process->total_runtime++;
    }
    
    // If time slice expired or process is blocked, find next READY process
    if (current_process->time_slice == 0 || current_process->state == PROCESS_BLOCKED) {
        // Reset time slice for current process
        if (current_process->state == PROCESS_READY || current_process->state == PROCESS_RUNNING) {
            current_process->time_slice = calculate_time_slice(current_process->priority);
            current_process->state = PROCESS_READY;
        }
        
        // Find next READY process using proper round-robin
        process_t *next = NULL;
        process_t *pos;
        int found = 0;
        
        // Start from the NEXT process in the list (proper round-robin)
        pos = list_entry(current_process->list.next, process_t, list);
        
        // Search from current->next to end of list
        while (&pos->list != &ready_queue) {
            if (pos != current_process && 
                (pos->state == PROCESS_READY || pos->state == PROCESS_RUNNING)) {
                next = pos;
                found = 1;
                break;
            }
            pos = list_entry(pos->list.next, process_t, list);
        }
        
        // If not found, wrap around and search from beginning to current
        if (!found) {
            list_for_each_entry(pos, &ready_queue, list) {
                if (pos == current_process) break;
                if (pos->state == PROCESS_READY || pos->state == PROCESS_RUNNING) {
                    next = pos;
                    found = 1;
                    break;
                }
            }
        }
        
        // If no other ready process found, keep running current
        if (!found || next == NULL) {
            current_process->state = PROCESS_RUNNING;
            return;
        }
        
        // Don't switch if we're switching to ourselves
        if (next == current_process) {
            current_process->state = PROCESS_RUNNING;
            return;
        }
        
        // Update states
        if (current_process->state == PROCESS_RUNNING) {
            current_process->state = PROCESS_READY;
        }
        next->state = PROCESS_RUNNING;
        next->time_slice = calculate_time_slice(next->priority);
        
        // Update TSS
        set_kernel_stack(next->kernel_stack_top);
        
        // Switch Page Directory
        if (next->cr3) {
            vmm_switch_directory(next->cr3);
        }
        
        // Context switch
        switch_to_task(next);
    } else {
        // Still have time slice, just update runtime
        current_process->state = PROCESS_RUNNING;
    }
}

void process_yield(void) {
    if (current_process) {
        current_process->time_slice = 0; // Force reschedule
    }
    schedule();
}

void process_debug_list(void) {
    pr_info("PID  | State\n");
    pr_info("---- | -----\n");
    
    if (list_empty(&ready_queue)) return;
    
    process_t *proc;
    list_for_each_entry(proc, &ready_queue, list) {
        pr_info("%d    | %s", proc->pid, 
            (proc->state == PROCESS_RUNNING) ? "RUNNING" :
            (proc->state == PROCESS_READY)   ? "READY" :
            (proc->state == PROCESS_BLOCKED) ? "BLOCKED" : "UNKNOWN");
            
        if (proc == current_process) pr_info(" (*)");
        pr_info("\n");
    }
}

int process_kill(uint32_t pid) {
    if (pid == 0) return 0; // Cannot kill kernel
    if (list_empty(&ready_queue)) return 0;
    
    process_t *proc, *tmp;
    list_for_each_entry_safe(proc, tmp, &ready_queue, list) {
        if (proc->pid == pid) {
            // Remove from list
            list_del(&proc->list);
            
            // Free process back to slab cache
            // NOTE: Memory leak here for stack/page directory.
            // Fixing memory leaks is a separate task.
            kmem_cache_free(process_cache, proc);
            
            // If we killed the running process, we MUST schedule immediately
            if (proc == current_process) {
                schedule();
                // We never return here if we switched away
            }
            return 1;
        }
    }
    
    return 0;
}

// ============================================================================
// NEW PROCESS MANAGEMENT FUNCTIONS
// ============================================================================

void process_set_priority(uint32_t pid, uint8_t priority) {
    if (list_empty(&ready_queue)) return;
    
    process_t *proc;
    list_for_each_entry(proc, &ready_queue, list) {
        if (proc->pid == pid) {
            proc->priority = priority;
            proc->time_slice = calculate_time_slice(priority);
            return;
        }
    }
}

void process_block(uint32_t pid) {
    if (list_empty(&ready_queue)) return;
    
    process_t *proc;
    list_for_each_entry(proc, &ready_queue, list) {
        if (proc->pid == pid) {
            proc->state = PROCESS_BLOCKED;
            // If blocking current process, force reschedule
            if (proc == current_process) {
                schedule();
            }
            return;
        }
    }
}

void process_unblock(uint32_t pid) {
    if (list_empty(&ready_queue)) return;
    
    process_t *proc;
    list_for_each_entry(proc, &ready_queue, list) {
        if (proc->pid == pid && proc->state == PROCESS_BLOCKED) {
            proc->state = PROCESS_READY;
            return;
        }
    }
}

void process_get_stats(uint32_t pid, uint32_t *runtime, uint8_t *priority, process_state_t *state) {
    if (list_empty(&ready_queue)) return;
    
    process_t *proc;
    list_for_each_entry(proc, &ready_queue, list) {
        if (proc->pid == pid) {
            if (runtime) *runtime = proc->total_runtime;
            if (priority) *priority = proc->priority;
            if (state) *state = proc->state;
            return;
        }
    }
}
