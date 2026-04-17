#pragma once

#define UART_BASE 0x10000000
#define UART_IER 0x01
#define UART_RBR 0x00
#define UART_LSR 0x05
#define UART_LSR_DR (1 << 0)

void uart_setup(void);
