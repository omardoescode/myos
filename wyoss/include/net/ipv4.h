#pragma once

#include "common/types.h"
#include "net/arp.h"
#include "net/ethernetframe.h"
namespace myos {
namespace net {
struct InternetProcotolV4Message {
  common::uint8_t headerLength : 4;
  common::uint8_t version : 4;
  common::uint8_t typeofservice;
  common::uint16_t totalLength;

  common::uint16_t identifier;
  common::uint16_t flagsAndOffset;

  common::uint8_t TimeToLive;
  common::uint8_t protocol;
  common::uint16_t checksum;

  common::uint32_t srcIP;
  common::uint32_t dstIP;
  // common::uint32_t options;
  // common::uint32_t options;
} __attribute__((packed));

class InternetProtocolProvider;
class InternetProtocolHandler {
protected:
  InternetProtocolProvider *provider;
  common::uint16_t protocol;

public:
  InternetProtocolHandler(InternetProtocolProvider *provider,
                          common::uint8_t protocol);
  ~InternetProtocolHandler();

  virtual bool onInternetProtocolReceived(
      common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
      common::uint8_t *internetProtocolPayload, common::uint32_t size);
  void Send(common::uint32_t dstIP_BE, common::uint8_t *internetProtocolPayload,
            common::uint32_t size);
};

class InternetProtocolProvider : public EtherFrameHandler {

  friend InternetProtocolHandler;

protected:
  InternetProtocolHandler *handlers[255];
  AddressResolutionProtocol *arp;
  common::uint32_t gatewayIP;
  common::uint32_t subnetMask;

public:
  InternetProtocolProvider(EtherFrameProvider *provider,
                           AddressResolutionProtocol *arp,
                           common::uint32_t gatewayIP,
                           common::uint32_t subnetMask);
  ~InternetProtocolProvider();

  bool onEtherFrameReceived(common::uint8_t *internetfrmaePayload,
                            common::uint32_t size) override;
  void Send(common::uint64_t dstIP_BE, common::uint8_t protocol,
            common::uint8_t *buffer, common::uint32_t size);

  static common::uint16_t Checksum(common::uint16_t *data,
                                   common::uint32_t lengthInBytes);
};
} // namespace net
} // namespace myos
