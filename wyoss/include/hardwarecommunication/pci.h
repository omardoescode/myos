#pragma once
#include <common/types.h>
#include <drivers/driver.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/port.h>

using namespace myos;
using namespace myos::hardwarecommunication;

namespace myos {
namespace hardwarecommunication {

enum BaseAddressRegisterType {
  MemoryMapping = 0,
  InputOutput = 1,
};

class BaseAddressRegister {
public:
  bool prefetchable;
  common::uint8_t *address;
  common::uint32_t size;
  BaseAddressRegisterType type;
};

class PeripheralComponentInterconnectControllerDeviceDescriptor {
public:
  common::uint32_t portBase;
  common::uint32_t interrupt;
  common::uint16_t bus;
  common::uint16_t device;
  common::uint16_t function;

  common::uint16_t vendor_id;
  common::uint16_t device_id;

  common::uint8_t class_id;
  common::uint8_t subclass_id;
  common::uint8_t interface_id;

  common::uint8_t revision;

  PeripheralComponentInterconnectControllerDeviceDescriptor();
  ~PeripheralComponentInterconnectControllerDeviceDescriptor();
};

class PeripheralComponentInterconnectController {
  Port32Bit dataPort;
  Port32Bit commandPort;

public:
  PeripheralComponentInterconnectController();
  ~PeripheralComponentInterconnectController();

  common::uint32_t Read(common::uint16_t bus, common::uint16_t device,
                        common::uint16_t function,
                        common::uint32_t registerOffset);

  void Write(common::uint16_t bus, common::uint16_t device,
             common::uint16_t function, common::uint32_t registerOffset,
             common::uint32_t vlaue);

  bool DeviceHasFunctions(common::uint16_t bus, common::uint16_t device);

  void SelectDrivers(drivers::DriverManager *drvMgr,
                     InterruptManager *interrupts);

  PeripheralComponentInterconnectControllerDeviceDescriptor
  GetDeviceDescriptor(common::uint16_t bus, common::uint16_t device,
                      common::uint16_t function);

  BaseAddressRegister GetBaseAddressRegister(common::uint16_t bus,
                                             common::uint16_t device,
                                             common::uint16_t function,
                                             common::uint16_t bar);
  drivers::Driver *
  GetDriver(PeripheralComponentInterconnectControllerDeviceDescriptor *drv,
            InterruptManager *interrupts);
};
} // namespace hardwarecommunication
} // namespace myos
