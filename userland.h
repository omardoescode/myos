#pragma once
// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x1000000

#define SSTATUS_SPIE (1 << 5)
#define SSTATUS_SUM (1 << 18)

void user_entry(void);
