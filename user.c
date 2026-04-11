#include "user.h"

extern char __stack_top[];

__attribute__((noreturn)) void exit(void) {
  for (;;)
    ;
}

void putchar(char ch) {
  // To be implemented via syscalls
  // I love syscalls :) [a cool concept]
}

__attribute__((section(".text.start"))) __attribute__((naked)) void
start(void) {
  __asm__ __volatile__("mv sp, %[stack_top] \n"
                       "call main           \n"
                       "call exit           \n" ::[stack_top] "r"(__stack_top));
}
