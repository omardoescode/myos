#pragma once

#include "common/types.h"
#include <hardwarecommunication/interrupts.h>

namespace myos {
namespace drivers {
class SyscallHandler : public hardwarecommunication::InterruptHandler {
public:
  SyscallHandler(hardwarecommunication::InterruptManager *interruptMgr,
                 common::uint8_t InterruptNumber);
  ~SyscallHandler();

  virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
};

} // namespace drivers
} // namespace myos
