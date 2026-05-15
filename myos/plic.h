#pragma once
#include "common.h"

#define PLIC_BASE 0x0C000000
#define PLIC_SIZE 0x600000

// UART is IRQ 10 (from device tree: serial@10000000 { interrupts = <0x0a> })
#define UART_IRQ 10

// S-mode context for hart 0 is 1 (even = M-mode, odd = S-mode)
#define PLIC_S_CONTEXT 1

// Priority: base + 0x4 * irq
// Each IRQ has a 32-bit priority register, spaced 4 bytes apart from base+0x0
// IRQ 10: 0x0 + 0x4 * 10 = 0x28
#define PLIC_PRIORITY(irq) (0x4 * (irq))

// Enable: base + 0x2000 + 0x80 * context
// Each context has 1024 enable bits (32 x 32-bit registers), spaced 0x80 apart
// S-mode context 1: 0x2000 + 0x80 * 1 = 0x2080
#define PLIC_ENABLE(context) (0x2000 + 0x80 * (context))

// Threshold: base + 0x200000 + 0x1000 * context
// Each context has its own threshold, spaced 0x1000 apart from 0x200000
// S-mode context 1: 0x200000 + 0x1000 * 1 = 0x201000
#define PLIC_THRESHOLD(context) (0x200000 + 0x1000 * (context))

// Claim/Complete: base + 0x200004 + 0x1000 * context
// Same block as threshold, offset +4. Read = claim, write = complete
// S-mode context 1: 0x200004 + 0x1000 * 1 = 0x201004
#define PLIC_CLAIM(context) (0x200004 + 0x1000 * (context))

void plic_setup(void);
uint32_t plic_claim(void);
void plic_complete(uint32_t irq);
