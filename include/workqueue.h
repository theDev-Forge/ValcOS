#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include "list.h"

/**
 * Linux-Style Work Queue System
 * 
 * Provides deferred/asynchronous work execution via kernel worker thread.
 */

// Work structure
struct work_struct {
    struct list_head list;
    void (*func)(struct work_struct *work);
    int pending;
};

/**
 * INIT_WORK - Initialize work structure
 * @work: Work to initialize
 * @func: Function to execute
 */
#define INIT_WORK(work, func_ptr) \
    do { \
        INIT_LIST_HEAD(&(work)->list); \
        (work)->func = (func_ptr); \
        (work)->pending = 0; \
    } while (0)

/**
 * schedule_work - Schedule work for execution
 * @work: Work to schedule
 * Returns: 1 if work was scheduled, 0 if already pending
 */
int schedule_work(struct work_struct *work);

/**
 * flush_work - Wait for work to complete
 * @work: Work to wait for
 */
void flush_work(struct work_struct *work);

/**
 * workqueue_init - Initialize work queue subsystem
 */
void workqueue_init(void);

/**
 * workqueue_get_pending_count - Get number of pending work items
 */
int workqueue_get_pending_count(void);

#endif /* WORKQUEUE_H */
