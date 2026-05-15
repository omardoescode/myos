#include "drivers/amd_am79c973.h"
#include "drivers/driver.h"
#include "memorymanagement.h"
#include <hardwarecommunication/pci.h>

void printf(const char *);
void printHex(common::uint8_t);

namespace myos {
namespace hardwarecommunication {

PeripheralComponentInterconnectControllerDeviceDescriptor::
    PeripheralComponentInterconnectControllerDeviceDescriptor() {}
PeripheralComponentInterconnectControllerDeviceDescriptor::
    ~PeripheralComponentInterconnectControllerDeviceDescriptor() {}
PeripheralComponentInterconnectController::
    PeripheralComponentInterconnectController()
    : dataPort(0xCFC), commandPort(0xCF8) {}
PeripheralComponentInterconnectController::
    ~PeripheralComponentInterconnectController() {}

common::uint32_t PeripheralComponentInterconnectController::Read(
    common::uint16_t bus, common::uint16_t device, common::uint16_t function,
    common::uint32_t registerOffset) {
  common::uint32_t id = (0x1 << 31) | ((bus & 0xFF) << 16) |
                        ((device & 0x1F) << 11) | ((function & 0x07) << 8) |
                        (registerOffset & 0xFC);
  commandPort.Write(id);
  common::uint32_t result = dataPort.Read();

  // NOTE: I listened to this twice, and didn't get it :(
  return result >> (8 * (registerOffset % 4));
}

void PeripheralComponentInterconnectController::Write(
    common::uint16_t bus, common::uint16_t device, common::uint16_t function,
    common::uint32_t registerOffset, common::uint32_t value) {

  common::uint32_t id = 0x1 << 31 | ((bus & 0xFF) << 16) |
                        ((device & 0x1F) << 11) | ((function & 0x07) << 8) |
                        (registerOffset & 0xFC);
  commandPort.Write(id);
  dataPort.Write(value);
}

bool PeripheralComponentInterconnectController::DeviceHasFunctions(
    common::uint16_t bus, common::uint16_t device) {
  return Read(bus, device, 0, 0x0E) & (1 << 7);
}

void PeripheralComponentInterconnectController::SelectDrivers(
    drivers::DriverManager *drvMgr, InterruptManager *interrupts) {

  for (int bus = 0; bus < 8; bus++) {
    for (int device = 0; device < 32; device++) {
      int numFunctions = DeviceHasFunctions(bus, device) ? 8 : 1;
      for (int func = 0; func < numFunctions; func++) {
        PeripheralComponentInterconnectControllerDeviceDescriptor dev =
            GetDeviceDescriptor(bus, device, func);

        if (dev.vendor_id == 0x0000 || dev.vendor_id == 0xFFFF)
          continue;

        for (int barNum = 0; barNum < 6; barNum++) {
          BaseAddressRegister bar =
              GetBaseAddressRegister(bus, device, func, barNum);

          // if set, we proceed
          if (bar.address && (bar.type == InputOutput))
            dev.portBase = (common::uint32_t)bar.address;
        }

        drivers::Driver *driver = GetDriver(&dev, interrupts);
        if (driver != 0)
          drvMgr->AddDriver(driver);

        // printf("PCI BUS ");
        // printHex(bus & 0xFF);
        // printf(", DEVICE ");
        // printHex(device & 0xFF);
        // printf(", FUNCTION ");
        // printHex(func & 0xFF);
        //
        // printf(", VENDOR ");
        // printHex((dev.vendor_id & 0xFF00) >> 8);
        // printHex(dev.vendor_id & 0xFF);
        //
        // printf(" ,DEVICE ");
        // printHex((dev.device_id & 0xFF00) >> 8);
        // printHex(dev.device_id & 0xFF);
        //
        // printf(" CLASS ");
        // printHex(dev.class_id & 0xFF);
        // printf(" SUB ");
        // printHex(dev.subclass_id & 0xFF);
        //
        // printf("\n");
      }
    }
  }
}

PeripheralComponentInterconnectControllerDeviceDescriptor
PeripheralComponentInterconnectController::GetDeviceDescriptor(
    common::uint16_t bus, common::uint16_t device, common::uint16_t function) {
  PeripheralComponentInterconnectControllerDeviceDescriptor result;

  result.bus = bus;
  result.device = device;
  result.function = function;

  result.vendor_id = Read(bus, device, function, 0x00);
  result.device_id = Read(bus, device, function, 0x02);

  result.class_id = Read(bus, device, function, 0x0B);
  result.subclass_id = Read(bus, device, function, 0x0a);
  result.interface_id = Read(bus, device, function, 0x09);
  result.revision = Read(bus, device, function, 0x08);
  result.interrupt = Read(bus, device, function, 0x3C);

  return result;
}

BaseAddressRegister
PeripheralComponentInterconnectController::GetBaseAddressRegister(
    common::uint16_t bus, common::uint16_t device, common::uint16_t function,
    common::uint16_t bar)

{
  BaseAddressRegister result;
  common::uint32_t headertype = Read(bus, device, function, 0x0E) & 0x7F;

  int maxBARS = 6 - (4 * headertype);
  if (bar >= maxBARS)
    return result; // address is null, useless value

  common::uint32_t bar_value = Read(bus, device, function, 0x10 + 4 * bar);
  result.type = (bar_value & 0x1) ? InputOutput : MemoryMapping;
  common::uint32_t temp;

  if (result.type == MemoryMapping) {
    result.prefetchable = ((bar_value >> 3) & 0x1) == 0x1;
    // TODO:
    switch ((bar_value >> 1) & 0x3) {
    case 0: // 32 bits Mode
    case 1: // 20 bits Mode
    case 2: // 64 bits Mode
      break;
    }
  } else {
    result.address = (common::uint8_t *)(bar_value & ~0x3);
    result.prefetchable = false;
  }

  return result;
}

drivers::Driver *PeripheralComponentInterconnectController::GetDriver(
    PeripheralComponentInterconnectControllerDeviceDescriptor *dev,
    InterruptManager *interrupts) {
  drivers::Driver *driver = 0;
  switch (dev->vendor_id) {
  case 0x1022: // AMD
    switch (dev->device_id) {
    case 0x2000: // am79c973
      printf("AMD am79c973 ");
      {
        common::uint32_t cmd = Read(dev->bus, dev->device, dev->function, 0x04);
        Write(dev->bus, dev->device, dev->function, 0x04, cmd | 0x07);
      }
      driver = (drivers::Driver *)MemoryManager::activeMemoryManager->malloc(
          sizeof(drivers::amd_am79c973));
      if (driver != 0) {
        new (driver) drivers::amd_am79c973(dev, interrupts);
        return driver;
      }
    }
    break;
  case 0x8086: // Intel
    break;
  }

  switch (dev->class_id) {
  case 0x03: // graphics
    switch (dev->subclass_id) {
    case 0x00: // VGA Graphics Devices
      // printf("VGA Graphics LOADED!!!\n");
      break;
    }
    break;
  }
  return driver;
}
} // namespace hardwarecommunication
} // namespace myos
