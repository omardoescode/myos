
#include "paging.h"
#include "common.h"
#include "panic.h"

extern char __free_ram[], __free_ram_end[];

int pages_bitmap[PAGE_COUNT];

paddr_t alloc_pages(uint32_t n) {
  size_t consecutive_zero = 0;
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    if (pages_bitmap[i] == 0)
      consecutive_zero++;
    else
      consecutive_zero = 0;

    // if we found pages
    if (consecutive_zero == n) {
      size_t start = i - n + 1;
      for (size_t j = start; j <= i; j++)
        pages_bitmap[j] = 1;
      paddr_t result = (paddr_t)__free_ram + start * PAGE_SIZE;
      memset((void *)result, 0, n * PAGE_SIZE);
      return result;
    }
  }

  PANIC("No space found");
}

void free_pages(paddr_t addr, uint32_t n) {
  if (!is_aligned(addr, PAGE_SIZE))
    PANIC("unaligned paddr %p", addr);

  if (addr < (paddr_t)__free_ram ||
      addr + n * PAGE_SIZE > (paddr_t)__free_ram_end)
    PANIC("Invalid page addres %p", addr);

  size_t idx = (addr - (paddr_t)__free_ram) / PAGE_SIZE;
  for (size_t i = 0; i < n; i++) {
    if (pages_bitmap[idx + i] == 0)
      PANIC("Double freeing at %p", addr + PAGE_SIZE * i);
    pages_bitmap[idx + i] = 0;
  }
}

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
  if (!is_aligned(vaddr, PAGE_SIZE))
    PANIC("unaligned vaddr %x", vaddr);

  if (!is_aligned(paddr, PAGE_SIZE))
    PANIC("unaligned paddr %x", paddr);

  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;

  if ((table1[vpn1] & PAGE_V) == 0) {
    // create the 1st level page table if it doesn't exist
    uint32_t pt_addr = alloc_pages(1);
    table1[vpn1] = ((pt_addr / PAGE_SIZE) << 10) | PAGE_V;
  }

  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
  uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
  table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

paddr_t unmap_page(uint32_t *table1, uint32_t vaddr) {
  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;

  paddr_t paddr = table1[vpn1];
  if (!(paddr & PAGE_V))
    return 0;

  uint32_t *table0 = (uint32_t *)((paddr >> 10) * PAGE_SIZE);
  if (!(table0[vpn0] & PAGE_V))
    return 0;

  uint32_t entry = table0[vpn0];
  table0[vpn0] = 0;
  paddr_t mapped = (entry >> 10) * PAGE_SIZE;
  return mapped;
}
