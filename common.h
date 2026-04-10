#pragma once

// types
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
typedef uint32_t size_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
typedef int bool;

// Support for booleans
#define true 1
#define false 0

// NULL
#define NULL ((void *)0)

// Compiler baked macros
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

// Output
void printf(const char *fmt, ...);
void puts(const char *str);
void putchar(char);

// Input
char getchar(void);

// Memory related methods
void *memset(void *s, char c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

// String methods
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
