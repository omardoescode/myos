#include "common/types.h"
#include "net/ipv4.h"
#include <net/icmp.h>

void printf(const char *);
void printHex(common::uint8_t);
namespace myos {
namespace net {

InternetControlMessageProtocol::InternetControlMessageProtocol(
    InternetProtocolProvider *provider)
    : InternetProtocolHandler(provider, 0x01) {}
InternetControlMessageProtocol::~InternetControlMessageProtocol() {}

bool InternetControlMessageProtocol::onInternetProtocolReceived(
    common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
    common::uint8_t *internetProtocolPayload, common::uint32_t size) {

  if (size < sizeof(InternetControlMessageProtocolMessage))
    return false;

  InternetControlMessageProtocolMessage *msg =
      (InternetControlMessageProtocolMessage *)internetProtocolPayload;

  switch (msg->type) {

  case 0:
    printf("ping response from ");
    printHex(srcIP_BE & 0xFF);
    printf(".");
    printHex((srcIP_BE >> 8) & 0xFF);
    printf(".");
    printHex((srcIP_BE >> 16) & 0xFF);
    printf(".");
    printHex((srcIP_BE >> 24) & 0xFF);
    printf("\n");
    break;
  case 8:
    msg->type = 0; // echo reply type
    msg->checksum = 0;
    msg->checksum = InternetProtocolProvider::Checksum(
        (common::uint16_t *)msg,
        sizeof(InternetControlMessageProtocolMessage));
    return true;
  };
  return false;
}

void InternetControlMessageProtocol::RequestEchoReply(common::uint32_t ip_be) {
  InternetControlMessageProtocolMessage icmp;
  icmp.type = 8; // ping
  icmp.code = 0;

  icmp.data = 0x3713; // 1337 (big endian version of leet)
  icmp.checksum = 0;
  icmp.checksum = InternetProtocolProvider::Checksum(
      (common::uint16_t *)&icmp, sizeof(InternetControlMessageProtocolMessage));

  InternetProtocolHandler::Send(ip_be, (common::uint8_t *)&icmp,
                                sizeof(InternetControlMessageProtocolMessage));
}
} // namespace net
} // namespace myos
