#pragma once

#include "common/types.h"
#include "net/ipv4.h"
namespace myos {
namespace net {

struct InternetControlMessageProtocolMessage {
  common::uint8_t type;
  common::uint8_t code;

  common::uint16_t checksum;
  common::uint32_t data;
} __attribute__((packed));

class InternetControlMessageProtocol : InternetProtocolHandler {
public:
  InternetControlMessageProtocol(InternetProtocolProvider *provider);
  ~InternetControlMessageProtocol();

  bool onInternetProtocolReceived(common::uint32_t srcIP_BE,
                                  common::uint32_t dstIP_BE,
                                  common::uint8_t *internetProtocolPayload,
                                  common::uint32_t size);

  void RequestEchoReply(common::uint32_t ip_be);
};
} // namespace net
} // namespace myos
