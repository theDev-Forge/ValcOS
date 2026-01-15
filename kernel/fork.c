#include "process.h"
#include "memory.h"
#include "string.h"
#include "printk.h"
#include "vmm.h"
#include "pmm.h"
#include "slab.h"

// Fork implementation - clone current process
int process_fork(void) {
    if (!current_process) {
        pr_err("fork: No current process\n");
        return -1;
    }
    
    // Allocate new process structure
    process_t *child = (process_t *)slab_alloc(process_cache);
    if (!child) {
        pr_err("fork: Failed to allocate process\n");
        return -1;
    }
    
    // Copy parent process data
    memcpy(child, current_process, sizeof(process_t));
    
    // Assign new PID
    static uint32_t next_pid = 2;
    child->pid = next_pid++;
    
    // Clone page directory
    child->cr3 = vmm_clone_directory();
    if (child->cr3 == 0) {
        slab_free(process_cache, child);
        pr_err("fork: Failed to clone page directory\n");
        return -1;
    }
    
    // Allocate new kernel stack
    child->kernel_stack_top = (uint32_t)kmalloc(4096) + 4096;
    
    // Copy kernel stack
    memcpy((void *)(child->kernel_stack_top - 4096),
           (void *)(current_process->kernel_stack_top - 4096),
           4096);
    
    // Reset state
    child->state = PROCESS_READY;
    child->time_slice = calculate_time_slice(child->priority);
    child->total_runtime = 0;
    
    // Clear pending signals
    child->pending_signals = 0;
    
    // Initialize list node
    INIT_LIST_HEAD(&child->list);
    
    // Add to ready queue
    list_add_tail(&child->list, &ready_queue);
    
    pr_info("fork: Created child process %d from parent %d\n", 
            child->pid, current_process->pid);
    
    // Return child PID to parent, 0 to child
    return child->pid;
}

// Wait for child process
int process_wait(int pid, int *status) {
    (void)pid;
    (void)status;
    
    // Simple implementation: just yield
    // Full implementation would block until child exits
    pr_warn("wait: Not fully implemented, yielding\n");
    process_yield();
    
    return 0;
}
