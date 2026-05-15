#pragma once
#include "common.h"
#define PANIC(fmt, ...)                                                        \
  do {                                                                         \
    printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);      \
    while (1)                                                                  \
      ;                                                                        \
  } while (0)

#define ASSERT(condition, fmt, ...)                                            \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("ASSERT: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
      while (1)                                                                \
        ;                                                                      \
    }                                                                          \
  } while (0)

#define TODO(todo)                                                             \
  do {                                                                         \
    printf("Not Implemented: %s", todo);                                       \
    while (1)                                                                  \
      ;                                                                        \
  } while (0)
