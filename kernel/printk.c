#include "printk.h"
#include "vga.h"
#include "string.h"

// Current log level (messages below this won't be displayed)
static int current_log_level = LOGLEVEL_INFO;

// Color mapping for each log level
static const log_color_t log_colors[] = {
    {VGA_COLOR_WHITE, VGA_COLOR_RED},        // EMERG   - White on Red
    {VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK},  // ALERT   - Bright Red
    {VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK},  // CRIT    - Bright Red
    {VGA_COLOR_RED, VGA_COLOR_BLACK},        // ERR     - Red
    {VGA_COLOR_YELLOW, VGA_COLOR_BLACK},     // WARNING - Yellow
    {VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK}, // NOTICE  - Cyan
    {VGA_COLOR_WHITE, VGA_COLOR_BLACK},      // INFO    - White
    {VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK}  // DEBUG   - Grey
};

// Helper: Extract log level from format string
static int extract_log_level(const char **fmt) {
    const char *s = *fmt;
    
    // Check for log level prefix: "<N>"
    if (s[0] == '<' && s[1] >= '0' && s[1] <= '7' && s[2] == '>') {
        int level = s[1] - '0';
        *fmt = s + 3;  // Skip the "<N>" prefix
        return level;
    }
    
    return LOGLEVEL_INFO;  // Default level
}

// Helper: Simple integer to string conversion
static int int_to_str(int value, char *buf, int base) {
    char temp[32];
    int i = 0;
    int is_negative = 0;
    
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }
    
    while (value > 0) {
        int digit = value % base;
        temp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        value /= base;
    }
    
    int len = 0;
    if (is_negative) buf[len++] = '-';
    
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

// Helper: Unsigned integer to string
static int uint_to_str(unsigned int value, char *buf, int base) {
    char temp[32];
    int i = 0;
    
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    
    while (value > 0) {
        int digit = value % base;
        temp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        value /= base;
    }
    
    int len = 0;
    while (i > 0) {
        buf[len++] = temp[--i];
    }
    buf[len] = '\0';
    
    return len;
}

// Helper: Format and print string
static void vprintk_internal(int level, const char *fmt, va_list args) {
    char buffer[512];
    int buf_pos = 0;
    
    // Set color based on log level
    if (level >= 0 && level <= 7) {
        vga_set_color(log_colors[level].fg, log_colors[level].bg);
    }
    
    // Process format string
    while (*fmt && buf_pos < (int)sizeof(buffer) - 1) {
        if (*fmt == '%') {
            fmt++;
            
            // Handle format specifiers
            switch (*fmt) {
                case 'd':  // Signed decimal
                case 'i': {
                    int val = va_arg(args, int);
                    char num_buf[32];
                    int_to_str(val, num_buf, 10);
                    for (char *p = num_buf; *p && buf_pos < (int)sizeof(buffer) - 1; p++) {
                        buffer[buf_pos++] = *p;
                    }
                    break;
                }
                
                case 'u': {  // Unsigned decimal
                    unsigned int val = va_arg(args, unsigned int);
                    char num_buf[32];
                    uint_to_str(val, num_buf, 10);
                    for (char *p = num_buf; *p && buf_pos < (int)sizeof(buffer) - 1; p++) {
                        buffer[buf_pos++] = *p;
                    }
                    break;
                }
                
                case 'x':  // Hexadecimal (lowercase)
                case 'X': {  // Hexadecimal (uppercase)
                    unsigned int val = va_arg(args, unsigned int);
                    char num_buf[32];
                    uint_to_str(val, num_buf, 16);
                    for (char *p = num_buf; *p && buf_pos < (int)sizeof(buffer) - 1; p++) {
                        buffer[buf_pos++] = (*fmt == 'X') ? (*p >= 'a' ? *p - 32 : *p) : *p;
                    }
                    break;
                }
                
                case 'p': {  // Pointer
                    buffer[buf_pos++] = '0';
                    buffer[buf_pos++] = 'x';
                    unsigned int val = (unsigned int)va_arg(args, void*);
                    char num_buf[32];
                    uint_to_str(val, num_buf, 16);
                    for (char *p = num_buf; *p && buf_pos < (int)sizeof(buffer) - 1; p++) {
                        buffer[buf_pos++] = *p;
                    }
                    break;
                }
                
                case 's': {  // String
                    char *str = va_arg(args, char*);
                    if (!str) str = "(null)";
                    while (*str && buf_pos < (int)sizeof(buffer) - 1) {
                        buffer[buf_pos++] = *str++;
                    }
                    break;
                }
                
                case 'c': {  // Character
                    buffer[buf_pos++] = (char)va_arg(args, int);
                    break;
                }
                
                case '%': {  // Literal %
                    buffer[buf_pos++] = '%';
                    break;
                }
                
                default:
                    // Unknown format, just print it
                    buffer[buf_pos++] = '%';
                    buffer[buf_pos++] = *fmt;
                    break;
            }
            fmt++;
        } else {
            buffer[buf_pos++] = *fmt++;
        }
    }
    
    buffer[buf_pos] = '\0';
    
    // Print the formatted string
    vga_print(buffer);
    
    // Reset color to default
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

int vprintk(const char *fmt, va_list args) {
    int level = extract_log_level(&fmt);
    
    // Filter based on log level
    if (level > current_log_level) {
        return 0;  // Message filtered out
    }
    
    vprintk_internal(level, fmt, args);
    return 0;
}

int printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintk(fmt, args);
    va_end(args);
    return ret;
}

void printk_set_level(int level) {
    if (level >= LOGLEVEL_EMERG && level <= LOGLEVEL_DEBUG) {
        current_log_level = level;
    }
}

int printk_get_level(void) {
    return current_log_level;
}
