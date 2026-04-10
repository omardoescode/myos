#include "kernel.h"
#include "common.h"

// __bss means the start address of the `.bss` section, so we use `[]` to ensure
// that _bss returns  an address and prevent any mistakes
extern char __bss[], __bss_end[], __stack_top[];

/*
 * Call OpenSBI as in the SBI specification
 */
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = fid;
  register long a7 __asm__("a7") = eid;

  __asm__ __volatile__("ecall"
                       : "=r"(a0), "=r"(a1)
                       : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                         "r"(a6), "r"(a7)
                       : "memory");

  return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
  // a7 = 1: Console Putchar, writes to the UART hardware
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

void kernel_main(void) {
  printf("\n\nHello %s\n", "World!");
  printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);
  printf("%x\n", -1);
  puts("Hello, World");

  for (;;) {
    __asm__ __volatile__("wfi");
  }
}

// place this in the `.text.boot` section in the linker script
// meaning that it should be placed at 0x80200000, which execution starts at
// (specified by us in the script)
__attribute__((section(".text.boot")))
// instructs the compiler not to generate unnecessary code before and after the
// function body, such as return instructions: exact matching
__attribute__((naked))
// NOTE: The execution of the kernel starts here
void boot(void) {
  // Set the stack pointer to the end address of the stack area specified in the
  // linker script, then jump to `kernel_main` function
  // NOTE: We set the end address because it decrements, not the start address
  __asm__ __volatile__("mv sp, %[stack_top]\n"
                       "j kernel_main"
                       :
                       : [stack_top] "r"(__stack_top));
}
