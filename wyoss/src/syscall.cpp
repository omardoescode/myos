#include "multitasking.h"
#include <syscall.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;

void printf(const char *str);
void printHex(uint8_t);
namespace myos {
namespace drivers {
SyscallHandler::SyscallHandler(
    hardwarecommunication::InterruptManager *interruptMgr,
    common::uint8_t interruptNumber)
    : InterruptHandler(interruptMgr,
                       interruptNumber +
                           interruptMgr->HardwareInterruptOffset()) {}

SyscallHandler::~SyscallHandler() {}
common::uint32_t SyscallHandler::HandleInterrupt(common::uint32_t esp) {
  CPUState *cpu = (CPUState *)esp;
  switch (cpu->eax) {
  case 4:
    printf((char *)cpu->ebx);
    break;
  default:
    printf("handled??");
    break;
  }
  return esp;
}
} // namespace drivers
} // namespace myos
