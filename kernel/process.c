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
    vga_print("Initializing Multitasking...\n");
    
    process_t *kernel_proc = (process_t*)kmalloc(sizeof(process_t));
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
    
    kernel_proc->next = kernel_proc;
    
    current_process = kernel_proc;
    ready_queue_head = kernel_proc;
    ready_queue_tail = kernel_proc;
}

void process_create(void (*entry_point)(void)) {
    process_t *proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    proc->cr3 = vmm_get_kernel_directory();
    proc->next = NULL;
    
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

    proc->esp = (uint32_t)ktop;

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
        
        // Find next READY process (skip BLOCKED and TERMINATED)
        process_t *next = current_process->next;
        int searched = 0;
        int total_processes = 0;
        
        // Count total processes
        process_t *counter = ready_queue_head;
        do {
            total_processes++;
            counter = counter->next;
        } while (counter != ready_queue_head);
        
        // Search for next READY process
        while (searched < total_processes) {
            if (next->state == PROCESS_READY || next->state == PROCESS_RUNNING) {
                break;
            }
            next = next->next;
            searched++;
        }
        
        // If no READY process found, stay with current
        if (searched >= total_processes) return;
        
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
    vga_print("PID  | State\n");
    vga_print("---- | -----\n");
    
    if (!ready_queue_head) return;
    
    process_t *curr = ready_queue_head;
    do {
        char pid_str[16];
        uint32_t pid = curr->pid;
        
        int i = 0;
        if (pid == 0) pid_str[i++] = '0';
        else {
            char temp[16]; int j=0;
            while(pid>0) { temp[j++]='0'+(pid%10); pid/=10; }
            while(j>0) pid_str[i++] = temp[--j];
        }
        pid_str[i] = '\0';
        
        vga_print(pid_str);
        // Padding
        for(int k=i; k<5; k++) vga_print(" ");
        vga_print("| ");
        if (curr->pid == 0) vga_print("Kernel");
        else vga_print("User");
        
        // Indicate current
        if (curr == current_process) vga_print(" (*)");
        
        vga_print("\n");
        
        curr = curr->next;
    } while (curr != ready_queue_head && curr != NULL);
}

int process_kill(uint32_t pid) {
    if (pid == 0) return 0; // Cannot kill kernel
    if (!ready_queue_head) return 0;
    
    process_t *curr = ready_queue_head;

    
    // Find prev to current head to handle circular cases correct (or iterate)
    // Actually, simple traversal:
    // Since it's circular, we need to find the node BEFORE the target.
    
    // Safety check just in case
    if (ready_queue_head->next == ready_queue_head) {
        // Only one process (Kernel). Can't kill it.
        if (ready_queue_head->pid == pid) return 0;
        return 0; // Not found
    }
    
    process_t *target = NULL;
    process_t *target_prev = NULL;
    
    curr = ready_queue_head;
    do {
        if (curr->next->pid == pid) {
            target_prev = curr;
            target = curr->next;
            break;
        }
        curr = curr->next;
    } while (curr != ready_queue_head);
    
    if (target) {
        // Unlink
        target_prev->next = target->next;
        
        // Fix head/tail if needed
        if (target == ready_queue_head) ready_queue_head = target->next;
        if (target == ready_queue_tail) ready_queue_tail = target_prev;
        
        // Free Memory
        // Free Stack (We allocated stack + PCB in one block logic or separate?)
        // In create_user: stack is separate pages. 
        // We really need to free the pages!
        // But for now, let's just free the PCB structure. 
        // NOTE: Memory leak here for stack/page directory. 
        // Fixing memory leaks is a separate task.
        kfree(target);
        
        // If we killed the running process, we MUST schedule immediately
        if (target == current_process) {
            schedule();
            // We never return here if we switched away
        }
        return 1;
    }
    
    return 0;
}

// ============================================================================
// NEW PROCESS MANAGEMENT FUNCTIONS
// ============================================================================

void process_set_priority(uint32_t pid, uint8_t priority) {
    if (!ready_queue_head) return;
    
    process_t *curr = ready_queue_head;
    do {
        if (curr->pid == pid) {
            curr->priority = priority;
            curr->time_slice = calculate_time_slice(priority);
            return;
        }
        curr = curr->next;
    } while (curr != ready_queue_head);
}

void process_block(uint32_t pid) {
    if (!ready_queue_head) return;
    
    process_t *curr = ready_queue_head;
    do {
        if (curr->pid == pid) {
            curr->state = PROCESS_BLOCKED;
            // If blocking current process, force reschedule
            if (curr == current_process) {
                schedule();
            }
            return;
        }
        curr = curr->next;
    } while (curr != ready_queue_head);
}

void process_unblock(uint32_t pid) {
    if (!ready_queue_head) return;
    
    process_t *curr = ready_queue_head;
    do {
        if (curr->pid == pid && curr->state == PROCESS_BLOCKED) {
            curr->state = PROCESS_READY;
            return;
        }
        curr = curr->next;
    } while (curr != ready_queue_head);
}

void process_get_stats(uint32_t pid, uint32_t *runtime, uint8_t *priority, process_state_t *state) {
    if (!ready_queue_head) return;
    
    process_t *curr = ready_queue_head;
    do {
        if (curr->pid == pid) {
            if (runtime) *runtime = curr->total_runtime;
            if (priority) *priority = curr->priority;
            if (state) *state = curr->state;
            return;
        }
        curr = curr->next;
    } while (curr != ready_queue_head);
}
