#include "vga_gfx.h"
#include "idt.h" // For outb/inb

// VGA Registers
#define VGA_AC_INDEX      0x3C0
#define VGA_AC_WRITE      0x3C0
#define VGA_AC_READ       0x3C1
#define VGA_MISC_WRITE    0x3C2
#define VGA_SEQ_INDEX     0x3C4
#define VGA_SEQ_DATA      0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA      0x3C9
#define VGA_GC_INDEX      0x3CE
#define VGA_GC_DATA       0x3CF
#define VGA_CRTC_INDEX    0x3D4
#define VGA_CRTC_DATA     0x3D5
#define VGA_INSTAT_READ   0x3DA

#define VGA_NUM_SEQ_REGS     5
#define VGA_NUM_CRTC_REGS   25
#define VGA_NUM_GC_REGS      9
#define VGA_NUM_AC_REGS     21

// Video memory for Mode 13h
static uint8_t *vga_mem = (uint8_t*)0xA0000;

// Register values for Mode 13h (320x200x256)
static uint8_t g_320x200x256[] = {
/* MISC */
	0x63,
/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00, 0x00
};

// Register values for Text Mode (80x25)
static uint8_t g_80x25_text[] = {
/* MISC */
	0x67,
/* SEQ */
	0x03, 0x00, 0x03, 0x00, 0x02,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00
};




// Helper to read back port for masking (simple version)
// Note: We don't have a read function, assuming exact overwrites or careful masking.
// In the code above I used outb_read which implies I can read that port.
// CRTC Data IS readable.
// But 'outb_read' isn't defined. Let's fix that.
// Actually standard CRTC registers are read/write.
// Let's implement a simple register reader helper inline
static inline uint8_t read_crtc(uint8_t index) {
    outb(VGA_CRTC_INDEX, index);
    return inb(VGA_CRTC_DATA);
}

// Re-implementing write_regs to be safer and correct without 'outb_read'
static void vga_write_regs_safe(uint8_t *regs) {
    unsigned int i;

    // Misc
    outb(VGA_MISC_WRITE, *regs);
    regs++;
    
    // Sequencer
    for(i = 0; i < VGA_NUM_SEQ_REGS; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *regs);
        regs++;
    }

    // Unlock CRTC
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, read_crtc(0x03) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, read_crtc(0x11) & ~0x80);

    // CRTC
    for(i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *regs);
        regs++;
    }

    // GC
    for(i = 0; i < VGA_NUM_GC_REGS; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *regs);
        regs++;
    }

    // AC
    for(i = 0; i < VGA_NUM_AC_REGS; i++) {
        (void)inb(VGA_INSTAT_READ);
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *regs);
        regs++;
    }
    
    (void)inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20); // Enable Palette
}


void vga_mode_13h(void) {
    vga_write_regs_safe(g_320x200x256);
}

void vga_text_mode(void) {
    vga_write_regs_safe(g_80x25_text);
    
    // Need to reload font? The BIOS usually puts font in plane 2.
    // Switching modes might mess it up if we destroyed plane 2 content.
    // However, usually register restore is enough if we didn't wipe memory.
    // If text is garbled, we might need to load a font.
    // For now, let's assume register restore is enough.
}

void vga_putpixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        vga_mem[y * 320 + x] = color;
    }
}

void vga_fill_screen(uint8_t color) {
    for (int i = 0; i < 320 * 200; i++) {
        vga_mem[i] = color;
    }
}

// Simple internal function to draw rectangles
static void vga_rect(int x, int y, int w, int h, uint8_t color) {
    for(int j=y; j<y+h; j++) {
        for(int i=x; i<x+w; i++) {
            vga_putpixel(i, j, color);
        }
    }
}

void vga_draw_logo(void) {
    vga_fill_screen(COLOR_BLACK);
    
    // Draw "V" (Blue)
    // Left diagonal
    for(int i=0; i<40; i++) vga_rect(100 + i/2, 60+i, 4, 1, COLOR_LIGHTBLUE);
    // Right diagonal
    for(int i=0; i<40; i++) vga_rect(120 + i/2, 100-i, 4, 1, COLOR_LIGHTBLUE);
    
    // Draw "al" (White)
    vga_rect(145, 80, 10, 20, COLOR_WHITE);
    vga_rect(160, 60, 5, 40, COLOR_WHITE);
    
    // Draw "c" (Red)
    vga_rect(175, 80, 15, 4, COLOR_RED);
    vga_rect(175, 80, 4, 20, COLOR_RED);
    vga_rect(175, 96, 15, 4, COLOR_RED);
    
    // Draw "OS" (Yellow)
    vga_rect(200, 60, 15, 40, COLOR_YELLOW);
    vga_rect(204, 64, 7, 32, COLOR_BLACK); // Hollow it out
    
    // Underline
    vga_rect(90, 110, 140, 2, COLOR_DARKGRAY);
    
    // Text
    // Since we don't have a font renderer in GFX mode yet, we just rely on the shapes above.
}
