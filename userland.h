#pragma once
// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x1000000

#define SSTATUS_SPIE (1 << 5)
#define SSTATUS_SUM (1 << 18)
#define SSTATUS_SIE (1 << 1)
#define SIE_SEIE (1 << 9)
#define SIE_STIE (1 << 5)
#define SIE_SSIE (1 << 1)

void user_entry(void);
