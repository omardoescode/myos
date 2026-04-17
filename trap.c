#include "trap.h"
#include "csr.h"
#include "fs.h"
#include "panic.h"
#include "process.h"
#include "syscall.h"

#define SCAUSE_ECALL 8

void handle_trap(struct trap_frame *f) {
  uint32_t scause = READ_CSR(scause);
  uint32_t stval = READ_CSR(stval);
  uint32_t user_pc = READ_CSR(sepc);

  if (scause & SCAUSE_INTERRUPT_BIT) {
    uint32_t masked = scause & 0x7FFFFFFF;
    if (masked == SCAUSE_S_EXTERNAL_INT) {
    }
  } else if (scause == SCAUSE_ECALL) {
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
  case SYS_READFILE:
  case SYS_WRITEFILE: {
    const char *filename = (const char *)f->a0;
    char *buf = (char *)f->a1;
    int len = (int)f->a2;
    struct file *file = fs_lookup(filename);

    if (!file) {
      printf("file not found: %s\n", filename);
      f->a0 = -1;
      break;
    }

    if (len > (int)sizeof(file->data))
      len = file->size;

    if (f->a3 == SYS_WRITEFILE) {
      memcpy(file->data, buf, len);
      file->size = len;
      fs_flush();
    } else {
      memcpy(buf, file->data, len);
    }

    f->a0 = len;
    break;
  }
  default:
    PANIC("unexpected syscall a3=%x\n", f->a3);
  }
}
