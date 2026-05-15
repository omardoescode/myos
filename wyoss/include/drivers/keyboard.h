#pragma once

#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>

namespace myos {
namespace drivers {

class KeyboardEventHandler {
public:
  KeyboardEventHandler();

  virtual void OnKeyDown(char);
  virtual void OnKeyUp(char);
};

class KeyboardDriver : public hardwarecommunication::InterruptHandler, public Driver {
private:
  hardwarecommunication::Port8Bit dataport;
  hardwarecommunication::Port8Bit commandport;

  KeyboardEventHandler *handler;

public:
  KeyboardDriver(hardwarecommunication::InterruptManager *manager, KeyboardEventHandler *handler);
  ~KeyboardDriver();
  virtual common::uint32_t HandleInterrupt(common::uint32_t esp);
  virtual void Activate();
};

}
}
