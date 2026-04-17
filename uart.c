#include "uart.h"
#include "common.h"
#include "panic.h"

#define RING_BUFFER_SIZE 128

// This will store character sent by the user, and interrupted by UART device
char ring_buffer[RING_BUFFER_SIZE];
int in = 0;
int out = 0;

void uart_setup() {
  *((volatile uint8_t *)(UART_BASE + UART_IER)) = 1;
  *((volatile uint8_t *)(UART_BASE + UART_FCR)) =
      UART_FCR_ENABLE | UART_FCR_CLEAR_RX | UART_FCR_CLEAR_TX;
}
bool uart_has_rx() {
  return *((volatile uint8_t *)(UART_BASE + UART_LSR)) & UART_LSR_DR;
}
char uart_read(void) { return *((volatile uint8_t *)(UART_BASE + UART_RBR)); }

void uart_push(char value) {
  if ((in + 1) % RING_BUFFER_SIZE == out)
    return; // we drop this character
  ring_buffer[in] = value;
  in = (in + 1) % RING_BUFFER_SIZE;
}

char uart_pop(void) {
  if (in == out)
    PANIC("uart_pop on empty ring");
  char value = ring_buffer[out];
  out = (out + 1) % RING_BUFFER_SIZE;
  return value;
}

bool uart_has_data(void) { return in != out; }
