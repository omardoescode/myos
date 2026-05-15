#include "common/types.h"
#include "drivers/amd_am79c973.h"
#include <memorymanagement.h>
#include <net/ethernetframe.h>

void printf(const char *);
void printHex(myos::common::uint8_t);

namespace myos {
namespace net {

EtherFrameHandler::EtherFrameHandler(EtherFrameProvider *provider,
                                     common::uint16_t etherType)
    : provider(provider), etherType_BE((((etherType & 0x00FF) << 8) |
                                        ((etherType & 0xFF00) >> 8))) {
  provider->handlers[etherType_BE] = this;
}

EtherFrameHandler::~EtherFrameHandler() {
  if (provider->handlers[etherType_BE] == this)
    provider->handlers[etherType_BE] = 0;
}

common::uint32_t EtherFrameHandler::GetIPAddress() {
  return provider->GetIPAddress();
}

bool EtherFrameHandler::onEtherFrameReceived(common::uint8_t *ehterframePayload,
                                             common::uint32_t size) {
  return false; // return nothing by default
}

void EtherFrameHandler::Send(common::uint64_t dstMAC_BE,
                             common::uint8_t *etherframePayload,
                             common::uint32_t size) {
  provider->Send(dstMAC_BE, etherType_BE, etherframePayload, size);
}

EtherFrameProvider::EtherFrameProvider(drivers::amd_am79c973 *backend)
    : drivers::RawDataHandler(backend) {

  for (common::uint32_t i = 0; i < 65535; i++)
    handlers[i] = 0;
}
EtherFrameProvider::~EtherFrameProvider() {}

bool EtherFrameProvider::OnRawDataReceived(common::uint8_t *buffer,
                                           common::uint32_t size) {

  if (size < sizeof(EtherFrameHeader))
    return false;

  EtherFrameHeader *frame = (EtherFrameHeader *)buffer;
  bool sendBack = false;

  if ((frame->dstMAC_BE == 0xFFFFFFFFFFFF) ||
      (frame->dstMAC_BE == backend->GetMacAddress())) {

    if (handlers[frame->etherType_BE] != 0)
      sendBack = handlers[frame->etherType_BE]->onEtherFrameReceived(
          buffer + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
  }

  if (sendBack) {
    // switch src and dst for the new frame
    frame->dstMAC_BE = frame->srcMAC_BE;
    frame->srcMAC_BE = backend->GetMacAddress();
  }

  return sendBack;
}
void EtherFrameProvider::Send(common::uint64_t dstMAC_BE,
                              common::uint16_t etherType_BE,
                              common::uint8_t *buffer, common::uint32_t size) {
  common::uint8_t *buffer2 =
      (common::uint8_t *)MemoryManager::activeMemoryManager->malloc(
          sizeof(EtherFrameHeader) + size);
  if (buffer2 == 0)
    return;

  EtherFrameHeader *frame = (EtherFrameHeader *)buffer2;
  frame->dstMAC_BE = dstMAC_BE;
  frame->srcMAC_BE = backend->GetMacAddress();
  frame->etherType_BE = etherType_BE;

  common::uint8_t *src = buffer;
  common::uint8_t *dst = buffer2 + sizeof(EtherFrameHeader);
  for (common::uint32_t i = 0; i < size; i++)
    dst[i] = src[i];

  backend->Send(buffer2, size + sizeof(EtherFrameHeader));
  MemoryManager::activeMemoryManager->free(buffer2);
}

common::uint32_t EtherFrameProvider::GetIPAddress() {
  return backend->GetIPAddress();
}

common::uint64_t EtherFrameProvider::GetMACAddress() {
  return backend->GetMacAddress();
}
} // namespace net

} // namespace myos
