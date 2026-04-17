#include "uart.h"
#include "common.h"

void uart_setup() { *((volatile uint8_t *)(UART_BASE + UART_IER)) = 1; }
