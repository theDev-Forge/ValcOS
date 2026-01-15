#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String length
size_t strlen(const char* str);

// String comparison
int strcmp(const char* str1, const char* str2);

// String copy
char* strcpy(char* dest, const char* src);

// String copy with length limit
char* strncpy(char* dest, const char* src, size_t n);

// String concatenate
char* strcat(char* dest, const char* src);

// Memory set
void* memset(void* ptr, int value, size_t num);

// Memory copy
void* memcpy(void* dest, const void* src, size_t num);

#endif

