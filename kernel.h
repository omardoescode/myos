#pragma once

/**
 * RISC-V Supervisor Binary Interface Specification
 */
struct sbiret {
  long error;
  long value;
};
