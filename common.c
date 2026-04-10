#include "common.h"
#include "sbi.h"

void printf(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case '\0':
        putchar('%');
        goto end;
      case '%':
        putchar('%');
        break;
      case 'c': {
        char c = (char)va_arg(vargs, int);
        putchar(c);
        break;
      }
      case 's': {
        const char *s = va_arg(vargs, const char *);
        while (*s) {
          putchar(*s);
          s++;
        }
        break;
      }
      case 'u': {
        unsigned value = va_arg(vargs, unsigned);
        unsigned divisor = 1;

        while (value / divisor > 9)
          divisor *= 10;
        while (divisor > 0) {
          putchar('0' + value / divisor);
          value %= divisor;
          divisor /= 10;
        }
        break;
      }
      case 'd': {
        int value = va_arg(vargs, int);
        unsigned magnitude = value;
        if (value < 0) {
          putchar('-');
          magnitude = -magnitude;
        }

        unsigned divisor = 1;
        while (magnitude / divisor > 9)
          divisor *= 10;

        while (divisor > 0) {
          putchar('0' + magnitude / divisor);
          magnitude %= divisor;
          divisor /= 10;
        }
        break;
      }
      case 'x': {
        unsigned value = va_arg(vargs, unsigned);
        for (int i = 7; i >= 0; i--) {
          unsigned nibble = (value >> (i * 4)) & 0xf;
          putchar("0123456789abcdef"[nibble]);
        }
      }
      }
    } else {
      putchar(*fmt);
    }

    fmt++;
  }

end:
  va_end(vargs);
}

void puts(const char *str) {
  while (*str != '\0') {
    putchar(*str);
    str++;
  }
  putchar('\n');
}

void *memset(void *s, char c, size_t n) {
  char *buf = (char *)s;
  for (size_t i = 0; i != n; i++)
    buf[i] = c;
  return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
  uint8_t *buf = (uint8_t *)dest;
  uint8_t *src_buf = (uint8_t *)src;
  for (size_t i = 0; i != n; i++) {
    buf[i] = src_buf[i];
  }
  return dest;
}

void putchar(char ch) {
  // a7 = 1: Console Putchar, writes to the UART hardware
  sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

char getchar(void) {
  char c = 0;
  for (;;) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    // this ecall puts the char in error
    if (ret.error != -1) {
      c = ret.error;
      break;
    }
  }
  return c;
}
