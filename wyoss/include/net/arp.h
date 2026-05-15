#pragma once

#include "common/types.h"
#include "net/ethernetframe.h"
namespace myos {
namespace net {

struct AddressResolutionProtocolMessage {
  common::uint16_t hardwareType;
  common::uint16_t protocol;
  common::uint8_t hardwareAddressSize; // 6
  common::uint8_t protocolAddressSize; // 4
  common::uint16_t command;

  common::uint64_t srcMAC : 48;
  common::uint32_t srcIP;
  common::uint64_t dstMAC : 48;
  common::uint32_t dstIP;
} __attribute__((packed));

class AddressResolutionProtocol : public EtherFrameHandler {
protected:
  common::uint32_t IPcache[128];
  common::uint64_t MACcache[128];
  int numCacheEntries;

public:
  AddressResolutionProtocol(EtherFrameProvider *provider);
  ~AddressResolutionProtocol();

  bool onEtherFrameReceived(common::uint8_t *ehterframePayload,
                            common::uint32_t size) override;

  void RequestMACAddress(common::uint32_t IP_BE);

  common::uint64_t GetMACFromCache(common::uint32_t IP_BE);
  common::uint64_t Resolve(common::uint32_t IP_BE);

  void BroadcastMacAddress(common::uint32_t IP_BE);
};
} // namespace net
} // namespace myos
