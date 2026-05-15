#include "user.h"
#include "syscall.h"

extern char __stack_top[];

__attribute__((noreturn)) void exit(void) {
  syscall(SYS_EXIT, 0, 0, 0);
  for (;;)
    ;
}

void putchar(char ch) { syscall(SYS_PUTCHAR, ch, 0, 0); }
char getchar(void) { return syscall(SYS_GETCHAR, 0, 0, 0); }

__attribute__((section(".text.start"))) __attribute__((naked)) void
start(void) {
  __asm__ __volatile__("mv sp, %[stack_top] \n"
                       "call main           \n"
                       "call exit           \n" ::[stack_top] "r"(__stack_top));
}

int readfile(const char *filename, char *buf, int len) {
  return syscall(SYS_READFILE, (int)filename, (int)buf, len);
}
int writefile(const char *filename, const char *buf, int len) {
  return syscall(SYS_WRITEFILE, (int)filename, (int)buf, len);
}
