#pragma once

#include "common/types.h"
#include "drivers/amd_am79c973.h"
namespace myos {
namespace net {

struct EtherFrameHeader {
  common::uint64_t dstMAC_BE : 48;
  common::uint64_t srcMAC_BE : 48;
  common::uint16_t etherType_BE;
} __attribute__((packed));

typedef common::uint32_t EtherFrameFooter;

class EtherFrameProvider;
class EtherFrameHandler {
protected:
  EtherFrameProvider *provider;
  common::uint16_t etherType_BE;

public:
  EtherFrameHandler(EtherFrameProvider *provider, common::uint16_t etherType);
  ~EtherFrameHandler();

  virtual bool onEtherFrameReceived(common::uint8_t *ehterframePayload,
                                    common::uint32_t size);
  void Send(common::uint64_t dstMAC_BE, common::uint8_t *ehterframePayload,
            common::uint32_t size);
  common::uint32_t GetIPAddress();
};

class EtherFrameProvider : public drivers::RawDataHandler {

  friend EtherFrameHandler;

protected:
  EtherFrameHandler *handlers[65535];

public:
  EtherFrameProvider(drivers::amd_am79c973 *backend);
  ~EtherFrameProvider();

  bool OnRawDataReceived(common::uint8_t *buffer, common::uint32_t size);
  void Send(common::uint64_t dstMAC_BE, common::uint16_t etherType_BE,
            common::uint8_t *buffer, common::uint32_t size);
  common::uint32_t GetIPAddress();
  common::uint64_t GetMACAddress();
};
} // namespace net
} // namespace myos
