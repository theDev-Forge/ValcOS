#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <stdint.h>

// VGA Colors (Mode 13h)
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHTGRAY 7
#define COLOR_DARKGRAY 8
#define COLOR_LIGHTBLUE 9
#define COLOR_LIGHTGREEN 10
#define COLOR_LIGHTCYAN 11
#define COLOR_LIGHTRED 12
#define COLOR_LIGHTMAGENTA 13
#define COLOR_YELLOW 14
#define COLOR_WHITE 15

// Resolution
#define GFX_WIDTH 320
#define GFX_HEIGHT 200

// Functions
void vga_mode_13h(void);
void vga_text_mode(void);
void vga_putpixel(int x, int y, uint8_t color);
void vga_fill_screen(uint8_t color);
void vga_draw_logo(void);

#endif
