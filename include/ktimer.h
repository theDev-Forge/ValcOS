#ifndef KTIMER_H
#define KTIMER_H

#include "list.h"
#include <stdint.h>

/**
 * Linux-Style Kernel Timer System
 * 
 * Provides delayed execution of functions using timer lists.
 * Separate from the basic timer.h (PIT timer driver).
 */

// Global jiffies counter (incremented on each timer tick)
extern volatile unsigned long jiffies;

// Kernel timer structure
struct ktimer_list {
    struct list_head list;
    unsigned long expires;     // Jiffies when timer should fire
    void (*function)(unsigned long);
    unsigned long data;
    int active;                // Is timer active?
};

/**
 * ktimer_init - Initialize a timer
 * @timer: Timer to initialize
 */
void ktimer_init(struct ktimer_list *timer);

/**
 * ktimer_add - Add timer to active list
 * @timer: Timer to add (must have expires and function set)
 */
void ktimer_add(struct ktimer_list *timer);

/**
 * ktimer_del - Remove timer from active list
 * @timer: Timer to remove
 * Returns: 1 if timer was active, 0 if not
 */
int ktimer_del(struct ktimer_list *timer);

/**
 * ktimer_mod - Modify timer expiration
 * @timer: Timer to modify
 * @expires: New expiration time
 * Returns: 1 if timer was active, 0 if not
 */
int ktimer_mod(struct ktimer_list *timer, unsigned long expires);

/**
 * ktimer_run - Check and run expired timers
 * Called from timer interrupt handler
 */
void ktimer_run(void);

/**
 * ktimer_subsystem_init - Initialize kernel timer subsystem
 */
void ktimer_subsystem_init(void);

// Helper macros
#define ktimer_setup(timer, fn, data_val) \
    do { \
        ktimer_init(timer); \
        (timer)->function = (fn); \
        (timer)->data = (data_val); \
    } while (0)

// Time conversion helpers
#define HZ 100  // Timer interrupt frequency (100 Hz = 10ms per tick)

static inline unsigned long msecs_to_jiffies(unsigned int msec) {
    return (msec * HZ) / 1000;
}

static inline unsigned long secs_to_jiffies(unsigned int sec) {
    return sec * HZ;
}

// Get active timer count
int ktimer_get_count(void);

#endif /* KTIMER_H */
