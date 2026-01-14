#include "vga.h"
#include "string.h"

static uint16_t* vga_buffer = (uint16_t*)0xB8000;
static size_t vga_row = 0;
static size_t vga_column = 0;
static uint8_t vga_current_color = 0;

void vga_init(void) {
    vga_current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = (uint16_t)' ' | (uint16_t)vga_current_color << 8;
        }
    }
    vga_row = 0;
    vga_column = 0;
}

void vga_set_color(uint8_t foreground, uint8_t background) {
    vga_current_color = vga_color(foreground, background);
}

void vga_putchar_at(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = (uint16_t)c | (uint16_t)color << 8;
}

static void vga_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            const size_t dst_index = y * VGA_WIDTH + x;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = (uint16_t)' ' | (uint16_t)vga_current_color << 8;
    }
    
    vga_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_column = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\b') {
        if (vga_column > 0) {
            vga_column--;
            vga_putchar_at(' ', vga_current_color, vga_column, vga_row);
        }
    } else {
        vga_putchar_at(c, vga_current_color, vga_column, vga_row);
        vga_column++;
    }
    
    if (vga_column >= VGA_WIDTH) {
        vga_column = 0;
        vga_row++;
    }
    
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void vga_print_color(const char* str, uint8_t color) {
    uint8_t old_color = vga_current_color;
    vga_current_color = color;
    vga_print(str);
    vga_current_color = old_color;
}
