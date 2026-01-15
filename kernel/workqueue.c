#include "workqueue.h"
#include "printk.h"
#include "process.h"

// Global work queue
static LIST_HEAD(work_queue);
static int pending_work_count = 0;

// Worker thread function
static void worker_thread(void) {
    pr_info("Work queue worker thread started\n");
    
    while (1) {
        struct work_struct *work = NULL;
        
        // Check if there's work to do
        if (!list_empty(&work_queue)) {
            work = list_first_entry(&work_queue, struct work_struct, list);
            list_del(&work->list);
            work->pending = 0;
            pending_work_count--;
            
            // Execute work
            if (work->func) {
                work->func(work);
            }
        }
        
        // Yield CPU if no work
        if (list_empty(&work_queue)) {
            process_yield();
        }
    }
}

void workqueue_init(void) {
    pr_info("Initializing work queue subsystem\n");
    
    // Create worker kernel thread
    process_create(worker_thread);
    
    pr_info("Work queue subsystem initialized\n");
}

int schedule_work(struct work_struct *work) {
    if (!work || !work->func) return 0;
    
    // Don't schedule if already pending
    if (work->pending) {
        return 0;
    }
    
    // Add to work queue
    list_add_tail(&work->list, &work_queue);
    work->pending = 1;
    pending_work_count++;
    
    return 1;
}

void flush_work(struct work_struct *work) {
    if (!work) return;
    
    // Simple implementation: wait until work is no longer pending
    while (work->pending) {
        process_yield();
    }
}

int workqueue_get_pending_count(void) {
    return pending_work_count;
}
