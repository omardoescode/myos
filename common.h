#pragma once

// types
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// Compiler baked macros
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void printf(const char *fmt, ...);
void puts(const char *str);

// Memory related methods
void *memset(void *s, char c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
