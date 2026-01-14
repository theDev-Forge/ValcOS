#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>

// VGA color codes
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

// VGA dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Initialize VGA driver
void vga_init(void);

// Clear the screen
void vga_clear(void);

// Put a character at a specific position
void vga_putchar_at(char c, uint8_t color, size_t x, size_t y);

// Put a character at the current cursor position
void vga_putchar(char c);

// Print a string
void vga_print(const char* str);

// Print a string with a specific color
void vga_print_color(const char* str, uint8_t color);

// Set text color
void vga_set_color(uint8_t foreground, uint8_t background);

// Create a color byte
static inline uint8_t vga_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// Draw splash screen (text mode)
void vga_draw_splash_text(void);

#endif
