#include "process.h"
#include "common.h"
#include "csr.h"
#include "kernel.h"
#include "paging.h"
#include "panic.h"
#include "plic.h"
#include "uart.h"
#include "userland.h"
#include "virtio.h"

extern char __kernel_base[], __free_ram_end[];

struct process procs[PROCS_MAX];
__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
  __asm__ __volatile__("addi sp, sp, -14 * 4\n"
                       "sw ra,  0  * 4(sp)\n"
                       "sw s0,  1  * 4(sp)\n"
                       "sw s1,  2  * 4(sp)\n"
                       "sw s2,  3  * 4(sp)\n"
                       "sw s3,  4  * 4(sp)\n"
                       "sw s4,  5  * 4(sp)\n"
                       "sw s5,  6  * 4(sp)\n"
                       "sw s6,  7  * 4(sp)\n"
                       "sw s7,  8  * 4(sp)\n"
                       "sw s8,  9  * 4(sp)\n"
                       "sw s9,  10 * 4(sp)\n"
                       "sw s10, 11 * 4(sp)\n"
                       "sw s11, 12 * 4(sp)\n"
                       "csrr t0, sstatus\n"
                       "sw t0,  13 * 4(sp)\n"

                       "sw sp, (a0)\n"
                       "lw sp, (a1)\n"

                       "lw t0,  13 * 4(sp)\n"
                       "csrw sstatus, t0\n"
                       "lw ra,  0  * 4(sp)\n"
                       "lw s0,  1  * 4(sp)\n"
                       "lw s1,  2  * 4(sp)\n"
                       "lw s2,  3  * 4(sp)\n"
                       "lw s3,  4  * 4(sp)\n"
                       "lw s4,  5  * 4(sp)\n"
                       "lw s5,  6  * 4(sp)\n"
                       "lw s6,  7  * 4(sp)\n"
                       "lw s7,  8  * 4(sp)\n"
                       "lw s8,  9  * 4(sp)\n"
                       "lw s9,  10 * 4(sp)\n"
                       "lw s10, 11 * 4(sp)\n"
                       "lw s11, 12 * 4(sp)\n"
                       "addi sp, sp, 14 * 4\n"
                       "ret\n");
}

struct process *create_process(const void *image, size_t image_size) {
  struct process *proc = NULL;
  int i;
  for (i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_UNUSED) {
      proc = &procs[i];
      break;
    }
  }

  if (!proc)
    PANIC("no free process slots");

  uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
  *--sp = SSTATUS_SIE;          // sstatus: start with interrupts enabled
  *--sp = 0;                    // s11
  *--sp = 0;                    // s10
  *--sp = 0;                    // s9
  *--sp = 0;                    // s8
  *--sp = 0;                    // s7
  *--sp = 0;                    // s6
  *--sp = 0;                    // s5
  *--sp = 0;                    // s4
  *--sp = 0;                    // s3
  *--sp = 0;                    // s2
  *--sp = 0;                    // s1
  *--sp = 0;                    // s0
  *--sp = (uint32_t)user_entry; // ra

  // Map Kernel Pages
  uint32_t *page_table = (uint32_t *)alloc_pages(1);
  for (paddr_t paddr = (paddr_t)__kernel_base; paddr < (paddr_t)__free_ram_end;
       paddr += PAGE_SIZE)
    map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
  map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, PAGE_R | PAGE_W);

  map_page(page_table, UART_BASE, UART_BASE, PAGE_R | PAGE_W);
  for (paddr_t p = PLIC_BASE; p < PLIC_BASE + PLIC_SIZE; p += PAGE_SIZE)
    map_page(page_table, p, p, PAGE_R | PAGE_W);

  // Map User pages
  for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
    paddr_t page = alloc_pages(1);

    size_t remaining = image_size - off;
    size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

    memcpy((void *)page, image + off, copy_size);
    map_page(page_table, USER_BASE + off, page,
             PAGE_U | PAGE_R | PAGE_W | PAGE_X);
  }

  // Initialize Fields
  proc->pid = i + 1;
  proc->state = PROC_RUNNABLE;
  proc->sp = (uint32_t)sp;
  proc->page_table = page_table;
  return proc;
}

struct process *current_proc;
struct process *idle_proc;

void yield(void) {
  WRITE_CSR(sstatus, READ_CSR(sstatus) | SSTATUS_SIE);
  struct process *next = idle_proc;
  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }

  if (next == current_proc)
    return;

  __asm__ __volatile__(
      "sfence.vma\n"
      "csrw satp, %[satp]\n"
      "sfence.vma\n"
      "csrw sscratch, %[sscratch]\n"
      :
      // Don't forget the trailing comma!
      : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
        [sscratch] "r"((uint32_t)&next->stack[sizeof(next->stack)]));

  // Clean up exited processes now that we've switched page tables
  for (int i = 0; i < PROCS_MAX; i++) {
    if (procs[i].state == PROC_EXITED) {
      destroy_process(&procs[i]);
      printf("Clean up process %d\n", i);
    }
  }

  struct process *prev = current_proc;
  current_proc = next;

  switch_context(&prev->sp, &next->sp);
}

void destroy_process(struct process *proc) {
  ASSERT(proc != NULL, "null pointer exception");

  for (size_t i = 0; i < 1024; i++) {
    if (!(proc->page_table[i] & PAGE_V))
      continue;

    uint32_t *table0 = (uint32_t *)((proc->page_table[i] >> 10) * PAGE_SIZE);
    for (size_t j = 0; j < 1024; j++) {
      if (!(table0[j] & PAGE_V))
        continue;

      if (table0[j] & PAGE_U)
        free_pages((table0[j] >> 10) * PAGE_SIZE, 1);
    }
    free_pages((paddr_t)table0, 1);

    proc->page_table[i] = 0;
  }
  free_pages((paddr_t)proc->page_table, 1);
  proc->state = PROC_UNUSED;
}

void wakeup_processes() {
  for (int i = 0; i < PROCS_MAX; i++)
    if (procs[i].state == PROC_WAITING)
      procs[i].state = PROC_RUNNABLE;
}

void sleep_current_process() { current_proc->state = PROC_WAITING; }
