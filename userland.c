#include "userland.h"
#include "panic.h"
__attribute__((naked)) void user_entry(void) {
  __asm__ __volatile__(
      "csrw sepc, %[sepc]        \n"
      "csrw sstatus, %[sstatus]  \n"
      "sret                      \n" // switch from S-mode to U-mode if spp bit
                                     // in sstatus is 0
      :
      : [sepc] "r"(USER_BASE), [sstatus] "r"(SSTATUS_SPIE));
}
