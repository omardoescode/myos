#include "common.h"
#include "process.h"
#include "sbi.h"
#include "uart.h"

void putchar(char ch) { sbi_call(ch, 0, 0, 0, 0, 0, 0, 1); }

char getchar(void) {
  while (!uart_has_data()) {
    sleep_current_process();
    yield();
  }
  return uart_pop();
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
