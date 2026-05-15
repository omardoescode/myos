#include "common/types.h"
#include "memorymanagement.h"
#include "net/arp.h"
#include "net/ethernetframe.h"
#include <net/ipv4.h>

namespace myos {
namespace net {

InternetProtocolHandler::InternetProtocolHandler(
    InternetProtocolProvider *provider, common::uint8_t protocol)
    : provider(provider), protocol(protocol) {
  provider->handlers[protocol] = this;
}
InternetProtocolHandler::~InternetProtocolHandler() {
  if (provider->handlers[protocol] == this)
    provider->handlers[protocol] = 0;
}

bool InternetProtocolHandler::onInternetProtocolReceived(
    common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
    common::uint8_t *internetProtocolPayload, common::uint32_t size) {
  return false;
}
void InternetProtocolHandler::Send(common::uint32_t dstIP_BE,
                                   common::uint8_t *internetProtocolPayload,
                                   common::uint32_t size) {
  provider->Send(dstIP_BE, protocol, internetProtocolPayload, size);
}

InternetProtocolProvider::InternetProtocolProvider(
    EtherFrameProvider *provider, AddressResolutionProtocol *arp,
    common::uint32_t gatewayIP, common::uint32_t subnetMask)
    : EtherFrameHandler(provider, 0x800), arp(arp), gatewayIP(gatewayIP),
      subnetMask(subnetMask) {
  for (int i = 0; i < 255; i++)
    handlers[i] = 0;
}

InternetProtocolProvider::~InternetProtocolProvider() {}

bool InternetProtocolProvider::onEtherFrameReceived(
    common::uint8_t *internetframePayload, common::uint32_t size) {
  if (size < sizeof(InternetProcotolV4Message))
    return false;

  InternetProcotolV4Message *ipmessage =
      (InternetProcotolV4Message *)internetframePayload;
  bool sendBack = false;

  if (ipmessage->dstIP == provider->GetIPAddress()) {

    int length = ipmessage->totalLength;
    if (length > size)
      length = size;

    if (handlers[ipmessage->protocol] != 0)
      sendBack = handlers[ipmessage->protocol]->onInternetProtocolReceived(
          ipmessage->srcIP, ipmessage->dstIP,
          internetframePayload + 4 * ipmessage->headerLength,
          length - 4 * ipmessage->headerLength);
  }

  if (sendBack) {
    // switch src and dst for the new frame
    common::uint32_t temp = ipmessage->dstIP;
    ipmessage->dstIP = ipmessage->srcIP;
    ipmessage->srcIP = temp;

    ipmessage->TimeToLive = 0x40;

    ipmessage->checksum = 0; // ??
    ipmessage->checksum =
        Checksum((common::uint16_t *)ipmessage, 4 * ipmessage->headerLength);
  }

  return sendBack;
}
void InternetProtocolProvider::Send(common::uint64_t dstIP_BE,
                                    common::uint8_t protocol,
                                    common::uint8_t *data,
                                    common::uint32_t size) {
  common::uint8_t *buffer =
      (common::uint8_t *)MemoryManager::activeMemoryManager->malloc(
          sizeof(InternetProcotolV4Message) + size);
  InternetProcotolV4Message *message = (InternetProcotolV4Message *)buffer;

  message->version = 4;
  message->headerLength = sizeof(InternetProcotolV4Message) / 4;
  message->typeofservice = 0;
  message->totalLength = size + sizeof(InternetProcotolV4Message);

  message->totalLength = ((message->totalLength & 0xFF00) >> 8) |
                         ((message->totalLength & 0x00FF) << 8);

  message->identifier = 0x0100;
  message->flagsAndOffset = 0x0040;
  message->TimeToLive = 0x40;
  message->protocol = protocol;

  message->dstIP = dstIP_BE;
  message->srcIP = provider->GetIPAddress();

  message->checksum = 0;
  message->checksum =
      Checksum((common::uint16_t *)message, 4 * message->headerLength);

  common::uint8_t *databuffer = buffer + sizeof(InternetProcotolV4Message);

  for (int i = 0; i < size; i++)
    databuffer[i] = data[i];

  common::uint32_t route = dstIP_BE;

  if ((dstIP_BE & subnetMask) != (message->srcIP & subnetMask))
    route = gatewayIP;

  provider->Send(arp->Resolve(route), this->etherType_BE, buffer,
                 size + sizeof(InternetProcotolV4Message));

  MemoryManager::activeMemoryManager->free(buffer);
}

common::uint16_t
InternetProtocolProvider::Checksum(common::uint16_t *data,
                                   common::uint32_t lengthInBytes) {
  common::uint32_t temp = 0;

  for (int i = 0; i < lengthInBytes / 2; i++)
    temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);

  if (lengthInBytes % 2 == 1)
    temp += ((common::uint16_t)((char *)data)[lengthInBytes - 1]) << 8;

  // keep adding the overflow bits
  while (temp & 0xFFFF0000)
    temp = (temp & 0xFFFF) + (temp >> 16);

  return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}
} // namespace net
} // namespace myos
