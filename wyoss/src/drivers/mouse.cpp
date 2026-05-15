#include "common/types.h"
#include <drivers/mouse.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

static uint16_t *VideoMemory = (uint16_t *)0xb8000;

MouseEventHandler::MouseEventHandler() {}

void MouseEventHandler::OnActivate() {}
void MouseEventHandler::OnMouseDown(uint8_t button) {}
void MouseEventHandler::OnMouseUp(uint8_t button) {}
void MouseEventHandler::OnMouseMove(int xoffset, int yoffset) {}

MouseDriver::MouseDriver(InterruptManager *manager, MouseEventHandler *handler)
    : InterruptHandler(manager, 0x2C), dataport(0x60), commandport(0x64),
      handler(handler) {}

MouseDriver::~MouseDriver() {}

void printf(const char *);
void printHex(uint32_t);

void MouseDriver::Activate() {
  VideoMemory[80 * 12 + 40] = ((VideoMemory[80 * 12 + 40] & 0xF000) >> 4 |
                               (VideoMemory[80 * 12 + 40] & 0x0F00) << 4 |
                               (VideoMemory[80 * 12 + 40] & 0x00FF));

  offset = 0;
  buttons = 0;
  commandport.Write(0xA8); // activate interrupts
  commandport.Write(0x20); // Get current state
  uint8_t status = (dataport.Read() | 2);

  commandport.Write(0x60); // set state
  dataport.Write(status);

  commandport.Write(0xD4);
  dataport.Write(0xF4);
  dataport.Read();

  if (handler != 0)
    handler->OnActivate();
}

uint32_t MouseDriver::HandleInterrupt(uint32_t esp) {
  uint8_t status = commandport.Read();
  if (!(status & 0x20))
    return esp;

  buffer[offset] = dataport.Read();
  offset = (offset + 1) % 3;

  if (handler == 0)
    return esp;

  if (offset == 0) {
    if (buffer[1] != 0 || buffer[2] != 0)
      handler->OnMouseMove((int8_t)buffer[1], -(int8_t)buffer[2]);

    for (uint8_t i = 0; i < 3; i++) {
      if ((buffer[0] & (0x01 << i)) != (buttons & (0x01 << i))) {
        // ... the button is pressed
        if (buttons & (0x1 << i))
          handler->OnMouseUp(i + 1);
        else
          handler->OnMouseDown(i + 1);
      }
    }
    buttons = buffer[0];
  }

  return esp;
}
