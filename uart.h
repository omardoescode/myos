#pragma once
#include "common.h"

#define UART_BASE 0x10000000
#define UART_IER 0x01
#define UART_RBR 0x00
#define UART_LSR 0x05
#define UART_LSR_DR (1 << 0)

void uart_setup(void);
char uart_read(void);

void uart_push(char);
char uart_pop(void);
bool uart_has_data(void);
