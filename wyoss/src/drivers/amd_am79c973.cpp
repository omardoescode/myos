#include "common/types.h"
#include "drivers/driver.h"
#include "hardwarecommunication/interrupts.h"
#include "hardwarecommunication/pci.h"
#include <drivers/amd_am79c973.h>

void printf(const char *str);
void printHex(common::uint8_t);
namespace myos {
namespace drivers {

namespace {
struct RxLogEntry {
  common::uint32_t size;
  common::uint8_t data[64];
};
constexpr int RX_LOG_CAP = 64;
RxLogEntry rxLog[RX_LOG_CAP];
volatile int rxHead = 0;
volatile int rxTail = 0;

void enqueueRx(common::uint8_t *buf, common::uint32_t size) {
  int next = (rxHead + 1) % RX_LOG_CAP;
  if (next == rxTail)
    return;
  common::uint32_t n = size > 64 ? 64 : size;
  rxLog[rxHead].size = n;
  for (common::uint32_t i = 0; i < n; i++)
    rxLog[rxHead].data[i] = buf[i];
  rxHead = next;
}
} // namespace

void DrainRxLog() {
  while (rxTail != rxHead) {
    printf("\nRECEIVING: ");
    RxLogEntry &e = rxLog[rxTail];
    for (common::uint32_t i = 34; i < e.size; i++) {
      printHex(e.data[i]);
      printf(" ");
    }
    rxTail = (rxTail + 1) % RX_LOG_CAP;
  }
}

RawDataHandler::RawDataHandler(amd_am79c973 *backend) : backend(backend) {
  backend->SetHandler(this);
}
RawDataHandler::~RawDataHandler() { backend->SetHandler(0); }

bool RawDataHandler::OnRawDataReceived(common::uint8_t *buffer,
                                       common::uint32_t size) {
  return false;
}

void RawDataHandler::Send(common::uint8_t *buffer, common::uint32_t size) {
  backend->Send(buffer, size);
}

amd_am79c973::amd_am79c973(
    hardwarecommunication::
        PeripheralComponentInterconnectControllerDeviceDescriptor *dev,
    hardwarecommunication::InterruptManager *InterruptMgr)
    : Driver(), hardwarecommunication::InterruptHandler(
                    InterruptMgr,
                    dev->interrupt + InterruptMgr->HardwareInterruptOffset()),
      MACAddress0Port(dev->portBase), MACAddress2Port(dev->portBase + 0x02),
      MACAddress4Port(dev->portBase + 0x04),
      registerDataPort(dev->portBase + 0x10),
      registerAddressPort(dev->portBase + 0x12),
      resetPort(dev->portBase + 0x14),
      busControlRegisterDataPort(dev->portBase + 0x16) {
  handler = 0;

  currentSendBuffer = 0;
  currentRecvBuffer = 0;

  common::uint8_t MAC0 = MACAddress0Port.Read() % 256;
  common::uint8_t MAC1 = MACAddress0Port.Read() / 256;
  common::uint8_t MAC2 = MACAddress2Port.Read() % 256;
  common::uint8_t MAC3 = MACAddress2Port.Read() / 256;
  common::uint8_t MAC4 = MACAddress4Port.Read() % 256;
  common::uint8_t MAC5 = MACAddress4Port.Read() / 256;
  printf("\nMac Address: ");
  printHex(MAC0);
  printHex(MAC1);
  printHex(MAC2);
  printHex(MAC3);
  printHex(MAC4);
  printHex(MAC5);
  printf("\n");

  common::uint64_t MAC = (common::uint64_t)MAC5 << 40 |
                         (common::uint64_t)MAC4 << 32 | MAC3 << 24 |
                         MAC2 << 16 | MAC1 << 8 | MAC0;

  // 32 bit mode
  registerAddressPort.Write(20);
  busControlRegisterDataPort.Write(0x102);

  // STOP reset
  registerAddressPort.Write(0);
  registerDataPort.Write(0x04);

  // InitBlock
  initBlock.mode = 0x0000; // promiscuous mode = false
  initBlock.reserved1 = 0;
  initBlock.numSendBuffers = 3;
  initBlock.reserved2 = 0;
  initBlock.numRecvBuffers = 3;
  initBlock.physicalAddress = MAC;
  initBlock.reserved3 = 0;
  initBlock.logicalAddress = 0;

  sendBufferDescr = (BufferDescriptor *)((
      ((common::uint32_t)(&sendBufferDescMemory[0]) + 15) &
      ~((common::uint32_t)0xF)));
  initBlock.sendBufferDescrAddress = (common::uint32_t)sendBufferDescr;

  recvBufferDescr = (BufferDescriptor *)((
      ((common::uint32_t)(&recvBufferDescMemory[0]) + 15) &
      ~((common::uint32_t)0xF)));
  initBlock.recvBufferDescrAddress = (common::uint32_t)recvBufferDescr;

  for (common::uint8_t i = 0; i < 8; i++) {
    sendBufferDescr[i].address =
        (((common::uint32_t)&sendBuffers[i]) + 15) & ~(common::uint32_t)0xF;
    sendBufferDescr[i].flags = 0x7FF | 0xF000;
    sendBufferDescr[i].flags2 = 0;
    sendBufferDescr[i].avail = 0;

    recvBufferDescr[i].address =
        (((common::uint32_t)&recvBuffers[i]) + 15) & ~(common::uint32_t)0xF;
    recvBufferDescr[i].flags = 0xf7ff | 0x80000000;
    recvBufferDescr[i].flags2 = 0;
    recvBufferDescr[i].avail = 0;
  }

  registerAddressPort.Write(1);
  registerDataPort.Write((common::uint32_t)(&initBlock) & 0xFFFF);

  registerAddressPort.Write(2);
  registerDataPort.Write(((common::uint32_t)(&initBlock) >> 16) & 0xFFFF);
}

amd_am79c973::~amd_am79c973() {}

void amd_am79c973::Activate() {
  registerAddressPort.Write(0);
  registerDataPort.Write(0x41);

  registerAddressPort.Write(4);
  common::uint32_t temp = registerDataPort.Read();
  registerAddressPort.Write(4);
  registerDataPort.Write(temp | 0xC00);

  registerAddressPort.Write(0);
  registerDataPort.Write(0x42);
}

int amd_am79c973::Reset() {
  resetPort.Read();
  resetPort.Write(0);
  return 10; // wait for 10ms
}
common::uint32_t amd_am79c973::HandleInterrupt(common::uint32_t esp) {
  registerAddressPort.Write(0);
  common::uint32_t temp = registerDataPort.Read();

  if ((temp & 0x8000) == 0x8000)
    printf("AMD am79c973 ERROR\n");
  if ((temp & 0x2000) == 0x2000)
    printf("AMD am79c973 COLLISION ERROR\n");
  if ((temp & 0x1000) == 0x1000)
    printf("AMD am79c973 MISSED FRAME\n");
  if ((temp & 0x0800) == 0x0800)
    printf("AMD am79c973 MEMORY ERROR\n");
  if ((temp & 0x0400) == 0x0400)
    Receive();
  if ((temp & 0x0200) == 0x0200)
    printf(" SENT");

  registerDataPort.Write(temp);
  registerAddressPort.Write(0);

  if ((temp & 0x0100) == 0x0100)
    printf("AMD am79c973 INIT DONE\n");

  return esp;
}

void amd_am79c973::Send(common::uint8_t *buffer, int size) {
  int sendDescriptor = currentSendBuffer;
  currentSendBuffer = (currentSendBuffer + 1) % 8;

  if (size > 1518)
    size = 1518; // discard overflow

  for (common::uint8_t *
           src = buffer + size - 1,
          *dst = (common::uint8_t *)(sendBufferDescr[sendDescriptor].address +
                                     size - 1);
       src >= buffer; src--, dst--) {
    *dst = *src;
  }

  printf("\nSENDING: ");
  for (int i = 34; i < (size > 64 ? 64 : size); i++) {
    printHex(buffer[i]);
    printf(" ");
  }

  sendBufferDescr[sendDescriptor].avail = 0;
  sendBufferDescr[sendDescriptor].flags2 = 0;
  sendBufferDescr[sendDescriptor].flags =
      0x8300F000 | ((common::uint16_t)((-size) & 0xFFF));
  registerAddressPort.Write(0);
  registerDataPort.Write(0x48);
}
void amd_am79c973::Receive() {
  for (; (recvBufferDescr[currentRecvBuffer].flags & 0x80000000) == 0;
       currentRecvBuffer = (currentRecvBuffer + 1) % 8) {
    if (!(recvBufferDescr[currentRecvBuffer].flags & 0x40000000) &&
        (recvBufferDescr[currentRecvBuffer].flags & 0x03000000) == 0x03000000) {
      common::uint32_t size = recvBufferDescr[currentRecvBuffer].flags2 & 0xFFF;
      if (size > 64)
        size -= 4;

      common::uint8_t *buffer =
          (common::uint8_t *)(recvBufferDescr[currentRecvBuffer].address);

      enqueueRx(buffer, size);

      if (handler != 0)
        if (handler->OnRawDataReceived(buffer, size))
          Send(buffer, size);
    }

    recvBufferDescr[currentRecvBuffer].flags2 = 0;
    recvBufferDescr[currentRecvBuffer].flags = 0x8000F7FF;
  }
}

void amd_am79c973::SetHandler(RawDataHandler *handler) {
  this->handler = handler;
}

common::uint64_t amd_am79c973::GetMacAddress() {
  return initBlock.physicalAddress;
}

common::uint32_t amd_am79c973::GetIPAddress() {
  return initBlock.logicalAddress;
}

void amd_am79c973::SetIPAddress(common::uint32_t ip) {
  initBlock.logicalAddress = ip;
}
} // namespace drivers
} // namespace myos
