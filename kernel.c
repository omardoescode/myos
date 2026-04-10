#include "kernel.h"
#include "common.h"
#include "panic.h"

// __bss means the start address of the `.bss` section, so we use `[]` to ensure
// that _bss returns  an address and prevent any mistakes
extern char __bss[], __bss_end[], __stack_top[];

void kernel_main(void) {
  printf("\n\nHello %s\n", "World!");
  printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);
  printf("%x\n", -1);
  puts("Hello, World");

  puts("Enter a character: ");
  char c = getchar();
  printf("Next Ascii Code: %c\n", c + 1);

  printf("Enter a string: ");
  char buf[148];
  printf("%p", &buf);
  readline(buf, sizeof(buf));
  printf("You typed: %s\n", buf);

  PANIC("booted!");
  puts("Unreachable");
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
