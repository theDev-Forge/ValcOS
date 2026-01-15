#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Timer callback function type
typedef void (*timer_callback_t)(void);

// Common timer frequencies
#define TIMER_FREQ_100HZ  100
#define TIMER_FREQ_1000HZ 1000

// Initialize timer with specified frequency
void init_timer(uint32_t frequency);

// Get current tick count
uint32_t timer_get_ticks(void);

// Get system uptime in milliseconds
uint32_t timer_get_uptime_ms(void);

// Wait for specified number of ticks (legacy)
void timer_wait(int ticks);

// Sleep for specified milliseconds
void timer_sleep_ms(uint32_t ms);

// Register a callback to be called at specified interval (in ticks)
// Returns callback ID, or -1 on failure
int timer_register_callback(timer_callback_t callback, uint32_t interval);

// Unregister a callback by ID
void timer_unregister_callback(int callback_id);

// Get timer statistics
void timer_get_stats(uint32_t *total_ticks, uint32_t *callbacks_executed);

#endif
