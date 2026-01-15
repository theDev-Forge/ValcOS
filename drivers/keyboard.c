#include "keyboard.h"
#include "idt.h"
#include "vga.h"

// Keyboard buffer
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

// Shift key state
static int shift_pressed = 0;

// US QWERTY keyboard layout
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Shifted characters
static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// External assembly interrupt handler
extern void keyboard_handler_asm(void);

static int extended_mode = 0;

// Keyboard scancodes for shift keys
#define SCANCODE_LSHIFT 0x2A
#define SCANCODE_RSHIFT 0x36

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Check for Extended Byte (E0)
    if (scancode == 0xE0) {
        extended_mode = 1;
        outb(0x20, 0x20);
        return;
    }
    
    // Handle shift key press
    if (scancode == SCANCODE_LSHIFT || scancode == SCANCODE_RSHIFT) {
        shift_pressed = 1;
        outb(0x20, 0x20);
        return;
    }
    
    // Handle shift key release
    if (scancode == (SCANCODE_LSHIFT | 0x80) || scancode == (SCANCODE_RSHIFT | 0x80)) {
        shift_pressed = 0;
        outb(0x20, 0x20);
        return;
    }
    
    // Handle Break Codes (Key Release)
    if (scancode & 0x80) {
        extended_mode = 0;
        outb(0x20, 0x20);
        return;
    }
    
    char ascii = 0;
    
    if (extended_mode) {
        extended_mode = 0;
        // Map Arrow Keys to special non-printable ASCII
        if (scancode == 0x48) ascii = 0x11;      // UP -> DC1
        else if (scancode == 0x50) ascii = 0x12; // DOWN -> DC2
    } else {
        // Standard ASCII mapping with shift support
        if (scancode < sizeof(scancode_to_ascii)) {
            if (shift_pressed) {
                ascii = scancode_to_ascii_shift[scancode];
            } else {
                ascii = scancode_to_ascii[scancode];
            }
        }
    }
    
    if (ascii != 0) {
        // Add to buffer
        keyboard_buffer[buffer_end] = ascii;
        buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    }
    
    // Send EOI (End of Interrupt) to PIC
    outb(0x20, 0x20);
}

void keyboard_init(void) {
    // Set up keyboard interrupt handler (IRQ1 = interrupt 33)
    idt_set_gate(33, (uint32_t)keyboard_handler_asm, 0x08, 0x8E);
}

char keyboard_getchar(void) {
    // Wait for a character
    while (buffer_start == buffer_end) {
        __asm__ volatile("hlt");
    }
    
    char c = keyboard_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int keyboard_available(void) {
    return buffer_start != buffer_end;
}
