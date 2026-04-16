#include "plic.h"
#include "common.h"

void plic_reg_write32(unsigned offset, uint32_t value) {
  *((volatile uint32_t *)(PLIC_BASE + offset)) = value;
}

uint32_t plic_reg_read32(unsigned offset) {
  return *((volatile uint32_t *)(PLIC_BASE + offset));
}
void setup_plic() {
  // Enable UART Interrupt
  plic_reg_write32(PLIC_PRIORITY(UART_IRQ), 1);

  // Enable UART S-Mode
  plic_reg_write32(PLIC_ENABLE(PLIC_S_CONTEXT), 1 << UART_IRQ);

  // Set threshold to 0
  plic_reg_write32(PLIC_THRESHOLD(PLIC_S_CONTEXT), 0);
}

uint32_t plic_claim(void) {
  return plic_reg_read32(PLIC_CLAIM(PLIC_S_CONTEXT));
}

void plic_complete(uint32_t irq) {
  plic_reg_write32(PLIC_CLAIM(PLIC_S_CONTEXT), irq);
}
