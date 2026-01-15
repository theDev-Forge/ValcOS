#include "ktimer.h"
#include "printk.h"
#include "string.h"

// Global jiffies counter
volatile unsigned long jiffies = 0;

// List of active timers
static LIST_HEAD(timer_list);

// Statistics
static int active_timer_count = 0;

void ktimer_subsystem_init(void) {
    jiffies = 0;
    pr_info("Kernel timer subsystem initialized\n");
}

void ktimer_init(struct ktimer_list *timer) {
    if (!timer) return;
    
    INIT_LIST_HEAD(&timer->list);
    timer->expires = 0;
    timer->function = NULL;
    timer->data = 0;
    timer->active = 0;
}

void ktimer_add(struct ktimer_list *timer) {
    if (!timer || !timer->function) return;
    
    if (timer->active) {
        pr_warn("ktimer_add: Timer already active\n");
        return;
    }
    
    // Add to timer list (unsorted for simplicity)
    list_add_tail(&timer->list, &timer_list);
    timer->active = 1;
    active_timer_count++;
}

int ktimer_del(struct ktimer_list *timer) {
    if (!timer) return 0;
    
    if (!timer->active) return 0;
    
    list_del(&timer->list);
    timer->active = 0;
    active_timer_count--;
    
    return 1;
}

int ktimer_mod(struct ktimer_list *timer, unsigned long expires) {
    if (!timer) return 0;
    
    int was_active = timer->active;
    
    if (was_active) {
        ktimer_del(timer);
    }
    
    timer->expires = expires;
    ktimer_add(timer);
    
    return was_active;
}

void ktimer_run(void) {
    struct ktimer_list *timer, *tmp;
    
    // Increment jiffies
    jiffies++;
    
    // Check for expired timers
    list_for_each_entry_safe(timer, tmp, &timer_list, list) {
        if (timer->expires <= jiffies) {
            // Timer expired, remove and execute
            list_del(&timer->list);
            timer->active = 0;
            active_timer_count--;
            
            // Execute callback
            if (timer->function) {
                timer->function(timer->data);
            }
        }
    }
}

int ktimer_get_count(void) {
    return active_timer_count;
}
