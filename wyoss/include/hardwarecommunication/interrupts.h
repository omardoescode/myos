#pragma once
#include "multitasking.h"
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/port.h>

namespace myos {
namespace hardwarecommunication {

class InterruptManager;

class InterruptHandler {
protected:
  common::uint8_t interruptNumber;
  InterruptManager *interruptManager;

  InterruptHandler(InterruptManager *interruptManager,
                   common::uint8_t interruptNumber);
  ~InterruptHandler();

public:
  virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
};

class InterruptManager {
  friend class InterruptHandler;

protected:
  static InterruptManager *ActiveInterruptManager;
  InterruptHandler *handlers[256];

  struct GateDescriptor {
    common::uint16_t handleAddressLowBits;
    common::uint16_t gdt_codeSegment;
    common::uint8_t reserved;
    common::uint8_t access;
    common::uint16_t handleAddressHighBits;
  } __attribute__((packed));

  static GateDescriptor interruptDescriptorTable[256];

  struct InterruptDescriptorTablePointer {
    common::uint16_t size;
    common::uint32_t base;
  } __attribute__((packed));

  common::uint16_t hardwareInterruptOffset;
  TaskManager *taskMgr;

  static void SetInterruptDescriptorTableEntry(
      common::uint8_t interruptNumber,
      common::uint16_t gdt_codeSegmentSelectorOffset, void (*handler)(),
      common::uint8_t DescriptorPrivilegeLevel, common::uint8_t DescriptorType);

  static void InterruptIgnore();

  static void HandleInterruptRequest0x00();
  static void HandleInterruptRequest0x01();
  static void HandleInterruptRequest0x02();
  static void HandleInterruptRequest0x03();
  static void HandleInterruptRequest0x04();
  static void HandleInterruptRequest0x05();
  static void HandleInterruptRequest0x06();
  static void HandleInterruptRequest0x07();
  static void HandleInterruptRequest0x08();
  static void HandleInterruptRequest0x09();
  static void HandleInterruptRequest0x0A();
  static void HandleInterruptRequest0x0B();
  static void HandleInterruptRequest0x0C();
  static void HandleInterruptRequest0x0D();
  static void HandleInterruptRequest0x0E();
  static void HandleInterruptRequest0x0F();
  static void HandleInterruptRequest0x31();

  static void HandleInterruptRequest0x80();

  static void HandleException0x00();
  static void HandleException0x01();
  static void HandleException0x02();
  static void HandleException0x03();
  static void HandleException0x04();
  static void HandleException0x05();
  static void HandleException0x06();
  static void HandleException0x07();
  static void HandleException0x08();
  static void HandleException0x09();
  static void HandleException0x0A();
  static void HandleException0x0B();
  static void HandleException0x0C();
  static void HandleException0x0D();
  static void HandleException0x0E();
  static void HandleException0x0F();
  static void HandleException0x10();
  static void HandleException0x11();
  static void HandleException0x12();
  static void HandleException0x13();

  static common::uint32_t handleInterrupt(common::uint8_t interruptNumber,
                                          common::uint32_t esp);
  common::uint32_t DoHandleInterrupt(common::uint8_t interruptNumber,
                                     common::uint32_t esp);

  Port8BitSlow picMasterCommand;
  Port8BitSlow picSlaveCommand;
  Port8BitSlow picMasterData;
  Port8BitSlow picSlaveData;

public:
  InterruptManager(common::uint16_t hardwareInterruptOffset,
                   myos::GlobalDescriptorTable *gdt, TaskManager *taskMgr);
  ~InterruptManager();
  common::uint16_t HardwareInterruptOffset();
  void Activate();
  void DeActivate();
};

} // namespace hardwarecommunication
} // namespace myos
