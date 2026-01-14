#include "keyboard.h"
#include "idt.h"
#include "vga.h"

// Keyboard buffer
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

// US QWERTY keyboard layout
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// External assembly interrupt handler
extern void keyboard_handler_asm(void);

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Only handle key press (not release)
    if (scancode & 0x80) {
        // Send EOI to PIC
        outb(0x20, 0x20);
        return;
    }
    
    // Convert scancode to ASCII
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            // Add to buffer
            keyboard_buffer[buffer_end] = c;
            buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
            
            // Display the character
            if (c == '\b') {
                vga_putchar('\b');
            } else if (c == '\n') {
                vga_putchar('\n');
            } else if (c != '\t') {
                vga_putchar(c);
            }
        }
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
