/*
 * Simple "Hello World" program for ValcOS
 * 
 * This is a minimal test program to verify ELF loading.
 * It should be compiled as a static binary with no dependencies.
 */

// For now, we'll use inline assembly to make a syscall
// In the future, this will use newlib

void _start(void) {
    // Simple infinite loop for testing
    // In a real program, this would print "Hello World" and exit
    
    while(1) {
        // Halt - in the future this will be exit(0)
        __asm__ volatile("hlt");
    }
}
