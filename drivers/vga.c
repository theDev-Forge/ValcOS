#include "vga.h"
#include "string.h"
#include "idt.h"

static uint16_t* vga_buffer = (uint16_t*)0xB8000;
static size_t vga_row = 0;
static size_t vga_column = 0;
static uint8_t vga_current_color = 0;

static void vga_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

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
    vga_update_cursor(0, 0);
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
    
    vga_update_cursor(vga_column, vga_row);
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

void vga_draw_splash_text(void) {
    vga_clear();
    
    // Simple ASCII Art Logo using colors
    // Centered roughly
    int start_y = 6;
    int start_x = 22;
    
    const char *logo[] = {
        "V     V    aaa    l       ccc    OOO    SSS ",
        "V     V   a   a   l      c      O   O  S    ",
        " V   V    aaaaa   l      c      O   O   SSS ",
        "  V V     a   a   l      c      O   O      S",
        "   V      a   a   lllll   ccc    OOO    SSS "
    };
    
    uint8_t colors[] = {
        VGA_COLOR_LIGHT_BLUE,
        VGA_COLOR_WHITE,
        VGA_COLOR_LIGHT_RED,
        VGA_COLOR_YELLOW,
        VGA_COLOR_LIGHT_GREEN
    };
    
    for (int i = 0; i < 5; i++) {
        const char *line = logo[i];
        int col_idx = 0;
        for (int j = 0; line[j] != 0; j++) {
            // rudimentary coloring based on position approx
            uint8_t c = VGA_COLOR_WHITE;
            if (j < 8) c = colors[0];       // V
            else if (j < 18) c = colors[1]; // a
            else if (j < 26) c = colors[1]; // l
            else if (j < 36) c = colors[2]; // c
            else if (j < 44) c = colors[3]; // O
            else c = colors[4];             // S
            
            // Draw direct to memory to avoid cursor move
             uint16_t *terminal = (uint16_t*)0xB8000;
             size_t index = (start_y + i) * VGA_WIDTH + (start_x + j);
             terminal[index] = (uint16_t)line[j] | (uint16_t)(c << 8);
        }
    }
    
    // Draw "Booting..." below
    const char *msg = "Initializing ValcOS...";
    int msg_len = 22;
    int msg_x = 40 - (msg_len/2);
    int msg_y = 15;
    
    for(int k=0; k<msg_len; k++) {
         uint16_t *terminal = (uint16_t*)0xB8000;
         size_t index = msg_y * VGA_WIDTH + (msg_x + k);
         terminal[index] = (uint16_t)msg[k] | (uint16_t)(VGA_COLOR_LIGHT_GREY << 8);
    }
}
