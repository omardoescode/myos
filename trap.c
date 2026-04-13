#include "trap.h"
#include "csr.h"
#include "panic.h"
#include "process.h"
#include "syscall.h"

#define SCAUSE_ECALL 8

void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);

  if (scause == SCAUSE_ECALL) {
    handle_syscall(f);
    user_pc += 4;
  } else {
    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
          user_pc);
  }

  WRITE_CSR(sepc, user_pc);
}

void handle_syscall(struct trap_frame *f) {
  switch (f->a3) {
  case SYS_PUTCHAR:
    putchar(f->a0);
    break;
  case SYS_GETCHAR:
    f->a0 = getchar();
    break;
  case SYS_EXIT:
    // NOTE: if we add fork/wait later, split into zombie (PROC_EXITED)
    // and reap (PROC_UNUSED via wait). For now, destroy in yield().
    printf("process %d exited\n", current_proc->pid);
    current_proc->state = PROC_EXITED;
    yield();
    PANIC("unreachable");
    break;
  default:
    PANIC("unexpected syscall a3=%x\n", f->a3);
  }
}
