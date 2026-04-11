#include "user.h"

void main(void) {
  // Try to do an interrupt
  /* *((volatile int *)0x80200000) = 0x1234; */
  for (;;)
    ;
}
