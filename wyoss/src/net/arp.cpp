#include "common/types.h"
#include "net/ethernetframe.h"
#include <net/arp.h>

namespace myos {
namespace net {

AddressResolutionProtocol::AddressResolutionProtocol(
    EtherFrameProvider *provider)
    : EtherFrameHandler(provider, 0x806), numCacheEntries(0) {}

AddressResolutionProtocol::~AddressResolutionProtocol() {}

bool AddressResolutionProtocol::onEtherFrameReceived(
    common::uint8_t *etherframePayload, common::uint32_t size) {
  if (size < sizeof(AddressResolutionProtocolMessage))
    return false;

  AddressResolutionProtocolMessage *arp =
      (AddressResolutionProtocolMessage *)etherframePayload;

  if (arp->hardwareType == 0x0100) {
    if (arp->protocol == 0x0008 && arp->hardwareAddressSize == 6 &&
        arp->protocolAddressSize == 4 &&
        arp->dstIP == provider->GetIPAddress()) {
      switch (arp->command) {
      case 0x0100: // request
        arp->command = 0x0200;
        arp->dstIP = arp->srcIP;
        arp->dstMAC = arp->srcMAC;
        arp->srcIP = provider->GetIPAddress();
        arp->srcMAC = provider->GetMACAddress();

        return true;
        break;
      case 0x0200: // response -> Put in cache
        if (numCacheEntries < 128) {
          IPcache[numCacheEntries] = arp->srcIP;
          MACcache[numCacheEntries] = arp->srcMAC;
          numCacheEntries++;
        }

        break;
      }
    }
  }
  return false;
}

void AddressResolutionProtocol::RequestMACAddress(common::uint32_t IP_BE) {
  AddressResolutionProtocolMessage arp;

  arp.hardwareType = 0x0100;   // ethernet
  arp.protocol = 0x0008;       // ipv4
  arp.hardwareAddressSize = 6; // mac
  arp.protocolAddressSize = 4; // ipv4
  arp.command = 0x0100;        // request

  arp.srcMAC = provider->GetMACAddress();
  arp.srcIP = provider->GetIPAddress();
  arp.dstMAC = 0xFFFFFFFFFFFF; // braodcast
  arp.dstIP = IP_BE;

  this->Send(arp.dstMAC, (common::uint8_t *)(&arp),
             sizeof(AddressResolutionProtocolMessage));
}

common::uint64_t
AddressResolutionProtocol::GetMACFromCache(common::uint32_t IP_BE) {
  for (int i = 0; i < numCacheEntries; i++)
    if (IPcache[i] == IP_BE)
      return MACcache[i];
  return 0xFFFFFFFFFFFF; // broadcast address
}

common::uint64_t AddressResolutionProtocol::Resolve(common::uint32_t IP_BE) {
  common::uint64_t result = GetMACFromCache(IP_BE);
  if (result == 0xFFFFFFFFFFFF)
    RequestMACAddress(IP_BE);

  // NOTE: This is not safe: If no internet, infinite loop. we can do a timeout
  while (result == 0xFFFFFFFFFFFF) {
    asm("hlt");
    result = GetMACFromCache(IP_BE);
  }

  return result;
}

void AddressResolutionProtocol::BroadcastMacAddress(common::uint32_t IP_BE) {
  AddressResolutionProtocolMessage arp;

  arp.hardwareType = 0x0100;   // ethernet
  arp.protocol = 0x0008;       // ipv4
  arp.hardwareAddressSize = 6; // mac
  arp.protocolAddressSize = 4; // ipv4
  arp.command = 0x0200;        // response

  arp.srcMAC = provider->GetMACAddress();
  arp.srcIP = provider->GetIPAddress();
  // arp.dstMAC = 0xFFFFFFFFFFFF; // braodcast
  arp.dstMAC = Resolve(IP_BE); // braodcast
  arp.dstIP = IP_BE;

  this->Send(arp.dstMAC, (common::uint8_t *)(&arp),
             sizeof(AddressResolutionProtocolMessage));
}
} // namespace net
} // namespace myos
