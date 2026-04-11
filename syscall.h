#pragma once

#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2
#define SYS_EXIT 3

int syscall(int sysno, int arg0, int arg1, int arg2);
