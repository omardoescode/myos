#include "common.h"
#include "sbi.h"

void putchar(char ch) {
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

char getchar(void) {
  char c = 0;
  for (;;) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    if (ret.error != -1) {
      c = ret.error;
      break;
    }
  }
  return c;
}

char *readline(char *buf, size_t n) {
  size_t i;

  for (i = 0; n - 1 > i; i++) {
    char c = getchar();
    putchar(c);
    if (c == '\n' || c == '\r') {
      buf[i] = '\0';
      break;
    }
    buf[i] = c;
  }
  buf[i] = '\0';
  putchar('\n');

  return buf;
}
