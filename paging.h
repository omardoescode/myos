#include "common.h"

#define SATP_SV32 (1u << 31)
#define PAGE_V (1 << 0) // "Valid" bit (entry is enabled)
#define PAGE_R (1 << 1) // Readable
#define PAGE_W (1 << 2) // Writable
#define PAGE_X (1 << 3) // Executable
#define PAGE_U (1 << 4) // User (accessible in user mode)

#define PAGE_SIZE 4096
#define PAGE_COUNT 2048

typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;
paddr_t alloc_pages(uint32_t n);
void free_pages(paddr_t addr, uint32_t n);

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);
paddr_t unmap_page(uint32_t *page_table, uint32_t vaddr);
