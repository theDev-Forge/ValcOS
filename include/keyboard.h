#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256

// Initialize keyboard driver
void keyboard_init(void);

// Get a character from the keyboard buffer (blocking)
char keyboard_getchar(void);

// Check if a key is available
int keyboard_available(void);

#endif
