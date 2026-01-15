#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>
#include <stdint.h>

/**
 * Linux-Style Kernel Logging (printk)
 * 
 * Provides formatted kernel logging with log levels and color-coded output.
 * Inspired by Linux kernel's printk subsystem.
 * 
 * Usage:
 *   printk(KERN_INFO "System initialized\n");
 *   pr_err("Failed to allocate memory\n");
 *   pr_warn("Low memory: %d KB free\n", free_mem);
 */

// Log Levels (from Linux kernel)
#define KERN_EMERG   "<0>"  // System is unusable
#define KERN_ALERT   "<1>"  // Action must be taken immediately
#define KERN_CRIT    "<2>"  // Critical conditions
#define KERN_ERR     "<3>"  // Error conditions
#define KERN_WARNING "<4>"  // Warning conditions
#define KERN_NOTICE  "<5>"  // Normal but significant condition
#define KERN_INFO    "<6>"  // Informational
#define KERN_DEBUG   "<7>"  // Debug-level messages

// Default log level (if no level specified)
#define KERN_DEFAULT KERN_INFO

// Log level values (for comparison)
#define LOGLEVEL_EMERG   0
#define LOGLEVEL_ALERT   1
#define LOGLEVEL_CRIT    2
#define LOGLEVEL_ERR     3
#define LOGLEVEL_WARNING 4
#define LOGLEVEL_NOTICE  5
#define LOGLEVEL_INFO    6
#define LOGLEVEL_DEBUG   7

/**
 * printk - Kernel logging with log levels
 * @fmt: Format string with optional log level prefix
 * 
 * Format: printk(KERN_LEVEL "message", args...)
 * Example: printk(KERN_ERR "Failed: %d\n", error_code);
 * 
 * If no log level is specified, KERN_DEFAULT is used.
 */
int printk(const char *fmt, ...);

/**
 * vprintk - printk with va_list
 * @fmt: Format string
 * @args: Variable argument list
 */
int vprintk(const char *fmt, va_list args);

// Convenience macros (like Linux pr_* macros)
#define pr_emerg(fmt, ...)   printk(KERN_EMERG fmt, ##__VA_ARGS__)
#define pr_alert(fmt, ...)   printk(KERN_ALERT fmt, ##__VA_ARGS__)
#define pr_crit(fmt, ...)    printk(KERN_CRIT fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)     printk(KERN_ERR fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)    printk(KERN_WARNING fmt, ##__VA_ARGS__)
#define pr_warning(fmt, ...) printk(KERN_WARNING fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...)  printk(KERN_NOTICE fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)    printk(KERN_INFO fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)   printk(KERN_DEBUG fmt, ##__VA_ARGS__)

/**
 * printk_set_level - Set minimum log level to display
 * @level: Minimum level (0-7)
 * 
 * Messages below this level will not be displayed.
 * Default: LOGLEVEL_INFO (6)
 */
void printk_set_level(int level);

/**
 * printk_get_level - Get current log level
 * 
 * Returns: Current minimum log level
 */
int printk_get_level(void);

// Color mapping for log levels
typedef struct {
    uint8_t fg;  // Foreground color
    uint8_t bg;  // Background color
} log_color_t;

#endif /* PRINTK_H */
