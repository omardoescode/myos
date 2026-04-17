#pragma once
#include "common.h"

// 16550-compatible UART
//
// Terminology:
//   RX = Receive  (host gets chars from outside, e.g. keypresses)
//   TX = Transmit (host sends chars out)
//
// Operating modes:
//   Character mode (FCR.ENABLE=0, legacy 16450): RBR and THR each hold ONE
//     byte. A second RX byte arriving before software reads the first
//     overwrites it (overrun error). Fast typing loses characters.
//   FIFO mode     (FCR.ENABLE=1, 16550+): 16-byte RX and TX FIFOs. Software
//     drains RX oldest-first via RBR; writes to THR enqueue TX. This kernel
//     uses FIFO mode.

#define UART_BASE 0x10000000

// Register offsets
#define UART_RBR 0x00 // Receive Buffer Register (read) — next RX byte
#define UART_IER 0x01 // Interrupt Enable Register (bit 0 = RX-data-available)
#define UART_FCR 0x02 // FIFO Control Register (write-only)
#define UART_LSR 0x05 // Line Status Register

// LSR bits
#define UART_LSR_DR (1 << 0) // Data Ready: at least one byte available in RX

// FCR bits
#define UART_FCR_ENABLE (1 << 0)   // Enable TX/RX FIFOs (switch to 16550 mode)
#define UART_FCR_CLEAR_RX (1 << 1) // Clear RX FIFO (self-clearing)
#define UART_FCR_CLEAR_TX (1 << 2) // Clear TX FIFO (self-clearing)

// Initialise the UART: enable RX-data-available interrupts and switch to
// FIFO mode (see "Operating modes" above).
void uart_setup(void);

// Read one byte from the UART's RX FIFO (RBR). Only safe to call when
// uart_has_rx() returned true; otherwise the byte is undefined.
char uart_read(void);

// True iff the UART hardware has at least one RX byte waiting (LSR.DR = 1).
// Used by the trap handler to drain the FIFO on every external interrupt,
// since one IRQ can cover multiple queued bytes.
bool uart_has_rx(void);

// Software ring buffer between the trap handler (producer) and getchar
// consumers. The hardware FIFO is only 16 bytes; this gives us more slack
// and decouples the interrupt path from sleeping readers.
void uart_push(char);
char uart_pop(void);
bool uart_has_data(void);
