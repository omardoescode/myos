#include <drivers/keyboard.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;

KeyboardEventHandler::KeyboardEventHandler() {}
void KeyboardEventHandler::OnKeyDown(char) {}
void KeyboardEventHandler::OnKeyUp(char) {}

KeyboardDriver::KeyboardDriver(InterruptManager *manager,
                               KeyboardEventHandler *handler)
    : InterruptHandler(manager, 0x21), dataport(0x60), commandport(0x64),
      handler(handler) {}

KeyboardDriver::~KeyboardDriver() {}

void printf(const char *);
void printHex(uint8_t);

void KeyboardDriver::Activate() {
  // Get rid of all keys done before initialization of the driver
  while (commandport.Read() & 0x1)
    dataport.Read();

  commandport.Write(0xAE); // activate interrupts

  commandport.Write(0x20); // Get current state
  uint8_t status = (dataport.Read() | 1) &
                   ~0x10; // set rightmost to 1, and undo the second bit

  commandport.Write(0x60); // set state
  dataport.Write(status);

  dataport.Write(0xF4);
  dataport.Read();
}

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp) {
  static bool shift = false;
  uint8_t key = dataport.Read();

  if (handler == 0)
    return esp;

  // Shift make/break must be handled before the `key < 0x80` filter,
  // since break codes are `make | 0x80`.
  switch (key) {
  case 0x2A:
  case 0x36:
    shift = true;
    return esp;
  case 0xAA:
  case 0xB6:
    shift = false;
    return esp;
  }

  if (key < 0x80) {
    switch (key) {
    case 0x02:
      handler->OnKeyDown(shift ? '!' : '1');
      break;
    case 0x03:
      handler->OnKeyDown(shift ? '@' : '2');
      break;
    case 0x04:
      handler->OnKeyDown(shift ? '#' : '3');
      break;
    case 0x05:
      handler->OnKeyDown(shift ? '$' : '4');
      break;
    case 0x06:
      handler->OnKeyDown(shift ? '%' : '5');
      break;
    case 0x07:
      handler->OnKeyDown(shift ? '^' : '6');
      break;
    case 0x08:
      handler->OnKeyDown(shift ? '&' : '7');
      break;
    case 0x09:
      handler->OnKeyDown(shift ? '*' : '8');
      break;
    case 0x0A:
      handler->OnKeyDown(shift ? '(' : '9');
      break;
    case 0x0B:
      handler->OnKeyDown(shift ? ')' : '0');
      break;
    case 0x0C:
      handler->OnKeyDown(shift ? '_' : '-');
      break;
    case 0x0D:
      handler->OnKeyDown(shift ? '+' : '=');
      break;
    case 0x0E:
      handler->OnKeyDown('\b');
      break; // Backspace
    case 0x0F:
      handler->OnKeyDown('\t');
      break; // Tab

    case 0x10:
      handler->OnKeyDown(shift ? 'Q' : 'q');
      break;
    case 0x11:
      handler->OnKeyDown(shift ? 'W' : 'w');
      break;
    case 0x12:
      handler->OnKeyDown(shift ? 'E' : 'e');
      break;
    case 0x13:
      handler->OnKeyDown(shift ? 'R' : 'r');
      break;
    case 0x14:
      handler->OnKeyDown(shift ? 'T' : 't');
      break;
    case 0x15:
      handler->OnKeyDown(shift ? 'Y' : 'y');
      break;
    case 0x16:
      handler->OnKeyDown(shift ? 'U' : 'u');
      break;
    case 0x17:
      handler->OnKeyDown(shift ? 'I' : 'i');
      break;
    case 0x18:
      handler->OnKeyDown(shift ? 'O' : 'o');
      break;
    case 0x19:
      handler->OnKeyDown(shift ? 'P' : 'p');
      break;
    case 0x1A:
      handler->OnKeyDown(shift ? '{' : '[');
      break;
    case 0x1B:
      handler->OnKeyDown(shift ? '}' : ']');
      break;
    case 0x1C:
      handler->OnKeyDown('\n');
      break; // Enter

    case 0x1E:
      handler->OnKeyDown(shift ? 'A' : 'a');
      break;
    case 0x1F:
      handler->OnKeyDown(shift ? 'S' : 's');
      break;
    case 0x20:
      handler->OnKeyDown(shift ? 'D' : 'd');
      break;
    case 0x21:
      handler->OnKeyDown(shift ? 'F' : 'f');
      break;
    case 0x22:
      handler->OnKeyDown(shift ? 'G' : 'g');
      break;
    case 0x23:
      handler->OnKeyDown(shift ? 'H' : 'h');
      break;
    case 0x24:
      handler->OnKeyDown(shift ? 'J' : 'j');
      break;
    case 0x25:
      handler->OnKeyDown(shift ? 'K' : 'k');
      break;
    case 0x26:
      handler->OnKeyDown(shift ? 'L' : 'l');
      break;
    case 0x27:
      handler->OnKeyDown(shift ? ':' : ';');
      break;
    case 0x28:
      handler->OnKeyDown(shift ? '"' : '\'');
      break;
    case 0x29:
      handler->OnKeyDown(shift ? '~' : '`');
      break;
    case 0x2B:
      handler->OnKeyDown(shift ? '|' : '\\');
      break;

    case 0x2C:
      handler->OnKeyDown(shift ? 'Z' : 'z');
      break;
    case 0x2D:
      handler->OnKeyDown(shift ? 'X' : 'x');
      break;
    case 0x2E:
      handler->OnKeyDown(shift ? 'C' : 'c');
      break;
    case 0x2F:
      handler->OnKeyDown(shift ? 'V' : 'v');
      break;
    case 0x30:
      handler->OnKeyDown(shift ? 'B' : 'b');
      break;
    case 0x31:
      handler->OnKeyDown(shift ? 'N' : 'n');
      break;
    case 0x32:
      handler->OnKeyDown(shift ? 'M' : 'm');
      break;
    case 0x33:
      handler->OnKeyDown(shift ? '<' : ',');
      break;
    case 0x34:
      handler->OnKeyDown(shift ? '>' : '.');
      break;
    case 0x35:
      handler->OnKeyDown(shift ? '?' : '/');
      break;

    case 0x39:
      handler->OnKeyDown(' ');
      break; // Space

    // Modifiers / locks / function keys — swallow
    case 0x01: // Esc
    case 0x1D: // LCtrl
    case 0x38: // LAlt
    case 0x3A: // CapsLock
    case 0x45: // NumLock
    case 0x46: // ScrollLock
      break;

    default:
      printf("KEYBOARD 0X");
      printHex(key);
    }
  }

  return esp;
}
