#pragma once

#include "common/types.h"
#include "net/ipv4.h"
#include <cstdint>
namespace myos {
namespace net {
struct UserDatagramProtocolHeader {
  common::uint16_t srcPort;
  common::uint16_t dstPort;
  common::uint16_t length;
  common::uint16_t checksum;
} __attribute__((packed));

class UserDatagramProtocolSocket;
class UserDatagramProtocolProvider;

class UserDatagramProtocolHandler {
public:
  UserDatagramProtocolHandler();
  ~UserDatagramProtocolHandler();

  virtual void
  HandlerUserDataProtocolMessage(UserDatagramProtocolSocket *socket,
                                 common::uint8_t *data, common::uint16_t size);
};

class UserDatagramProtocolSocket {
  friend class UserDatagramProtocolProvider;

protected:
  // src port -> the source sends to me. makes sense
  common::uint16_t remotePort;
  common::uint32_t remoteIP;
  // dst port
  common::uint16_t localPort;
  common::uint32_t localIP;

  bool listening;

  UserDatagramProtocolProvider *backend;
  UserDatagramProtocolHandler *handler;

public:
  UserDatagramProtocolSocket(UserDatagramProtocolProvider *backend);
  ~UserDatagramProtocolSocket();

  virtual void HandlerUserDataProtocolMessage(common::uint8_t *data,
                                              common::uint16_t size);

  virtual void Send(common::uint8_t *data, common::uint16_t size);
  virtual void Disconnect();
};

class UserDatagramProtocolProvider : public InternetProtocolHandler {
protected:
  UserDatagramProtocolSocket *sockets[65535];
  common::uint16_t freePort;
  common::uint16_t numSockets;

public:
  UserDatagramProtocolProvider(InternetProtocolProvider *provider);
  ~UserDatagramProtocolProvider();
  bool onInternetProtocolReceived(common::uint32_t srcIP_BE,
                                  common::uint32_t dstIP_BE,
                                  common::uint8_t *internetProtocolPayload,
                                  common::uint32_t size) override;

  virtual UserDatagramProtocolSocket *Connect(common::uint32_t ip,
                                              common::uint16_t port);
  virtual UserDatagramProtocolSocket *Listen(common::uint16_t port);
  virtual void Disconnect(UserDatagramProtocolSocket *socket);
  virtual void Send(UserDatagramProtocolSocket *socket, common::uint8_t *data,
                    common::uint16_t size);

  virtual void Bind(UserDatagramProtocolSocket *socket,
                    UserDatagramProtocolHandler *handler);
};
} // namespace net
} // namespace myos
