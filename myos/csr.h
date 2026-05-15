#pragma once
#define READ_CSR(reg)                                                          \
  ({                                                                           \
    unsigned long csr_tmp;                                                     \
    __asm__ __volatile__("csrr %0, " #reg : "=r"(csr_tmp));                    \
    csr_tmp;                                                                   \
  })

#define WRITE_CSR(reg, value)                                                  \
  do {                                                                         \
    uint32_t csr_tmp = (value);                                                \
    __asm__ __volatile__("csrw " #reg ", %0" ::"r"(csr_tmp));                  \
  } while (0)
