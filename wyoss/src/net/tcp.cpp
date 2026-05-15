#include "net/tcp.h"
#include "common/types.h"
#include "memorymanagement.h"
#include "net/ipv4.h"
#include <cstdint>

void printf(const char *);
namespace myos {
namespace drivers {
void DrainRxLog();
}
} // namespace myos

namespace myos {
namespace net {

uint32_t bigEndian32(common::uint32_t x) {
  return (x & 0xFF000000) >> 24 | (x & 0x00FF0000) >> 8 |
         (x & 0x0000FF00) << 8 | (x & 0x000000FF) << 24;
}

TransmissionControlProtocolHandler::TransmissionControlProtocolHandler() {}
TransmissionControlProtocolHandler::~TransmissionControlProtocolHandler() {}

bool TransmissionControlProtocolHandler::HandleTransmissionControlMessage(
    TransmissionControlProtocolSocket *socket, common::uint8_t *data,
    common::uint16_t size) {
  return true;
}

// Socket

TransmissionControlProtocolSocket::TransmissionControlProtocolSocket(
    TransmissionControlProtocolProvider *backend)
    : backend(backend), handler(0), state(CLOSED) {}
TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket() {}

bool TransmissionControlProtocolSocket::HandleTrasmissionControlMessage(
    common::uint8_t *data, common::uint16_t size) {
  if (handler != 0)
    return handler->HandleTransmissionControlMessage(this, data, size);
  return true;
}

void TransmissionControlProtocolSocket::Send(common::uint8_t *data,
                                             common::uint16_t size) {
  while (state != ESTABLISHED) {
    myos::drivers::DrainRxLog();
    asm("hlt");
  }
  myos::drivers::DrainRxLog();
  backend->Send(this, data, size, PSH | ACK);
}
void TransmissionControlProtocolSocket::Disconnect() {
  backend->Disconnect(this);
}

// PROVIDER

TransmissionControlProtocolProvider::TransmissionControlProtocolProvider(
    InternetProtocolProvider *provider)
    : InternetProtocolHandler(provider, 0x06), numSockets(0), freePort(1024) {
  for (int i = 0; i < 65535; i++)
    sockets[i] = 0;
}

TransmissionControlProtocolProvider::~TransmissionControlProtocolProvider() {}

bool TransmissionControlProtocolProvider::onInternetProtocolReceived(
    common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
    common::uint8_t *internetProtocolPayload, common::uint32_t size) {
  if (size < 20) // 20 is the smallest possible TCP header
    return false;

  TransmissionControlProtocolHeader *msg =
      (TransmissionControlProtocolHeader *)internetProtocolPayload;

  uint16_t localPort = msg->dstPort;
  uint16_t remotePort = msg->srcPort;

  TransmissionControlProtocolSocket *socket = 0;
  for (int i = 0; i < numSockets && socket == 0; i++) {
    if (sockets[i]->localPort == msg->dstPort &&
        sockets[i]->localIP == dstIP_BE && sockets[i]->state == LISTENING &&
        ((msg->flags) & (SYN | ACK)) == SYN)
      socket = sockets[i];
    else if (sockets[i]->localPort == msg->dstPort &&
             sockets[i]->localIP == dstIP_BE &&
             sockets[i]->remotePort == msg->srcPort &&
             sockets[i]->remoteIP == srcIP_BE)
      socket = sockets[i];
  }

  bool reset = false;

  if (socket != 0) {
    switch ((msg->flags) & (SYN | ACK | FIN)) {
    case SYN:
      if (socket->state == LISTENING) {
        socket->state = SYN_RECEIVED;
        socket->remotePort = msg->srcPort;
        socket->remoteIP = srcIP_BE;
        socket->acknowledgementNumber = bigEndian32(msg->sequenceNumber) + 1;
        socket->sequenceNumber = 0xbeefcafe;
        Send(socket, 0, 0, SYN | ACK);
        socket->sequenceNumber++;
        return false;
      } else
        reset = true;

      break;
    case SYN | ACK:

      if (socket->state == SYN_SENT) {
        socket->state = ESTABLISHED;
        socket->acknowledgementNumber = bigEndian32(msg->sequenceNumber) + 1;
        socket->sequenceNumber++;
        Send(socket, 0, 0, ACK);
      } else
        reset = true;

      break;
    case SYN | FIN:
    case SYN | ACK | FIN:
      reset = true;
      break;
    case FIN:
    case FIN | ACK:
      if (socket->state == ESTABLISHED) {
        socket->state = CLOSE_WAIT;
        socket->acknowledgementNumber++;
        Send(socket, 0, 0, ACK);
        Send(socket, 0, 0, FIN | ACK);
      } else if (socket->state == CLOSE_WAIT) {
        socket->state = CLOSED;
      } else if (socket->state == FIN_WAIT1 || socket->state == FIN_WAIT2) {
        socket->state = CLOSED;
        socket->acknowledgementNumber++;
        Send(socket, 0, 0, ACK);
      } else
        reset = true;
      break;

    case ACK:
      if (socket->state == SYN_RECEIVED) {
        socket->state = ESTABLISHED;
        return false;
      } else if (socket->state == FIN_WAIT1) {
        socket->state = FIN_WAIT2;
        return false;
      } else if (socket->state == CLOSE_WAIT) {
        socket->state = CLOSED;
        break;
      }

      if (msg->flags == ACK)
        break;

      // no break, because of piggybacking
    default:
      if (bigEndian32(msg->sequenceNumber) == socket->acknowledgementNumber) {
        reset = !socket->HandleTrasmissionControlMessage(
            internetProtocolPayload + msg->headerSize32 * 4,
            size - msg->headerSize32 * 4);

        if (!reset) {
          socket->acknowledgementNumber += size - msg->headerSize32 * 4;
          Send(socket, 0, 0, ACK);
        }
      } else {
        // data in wrong order
        // need to store
        reset = true;
      }
    }
  }

  if (reset) {
    if (socket != 0) {
      Send(socket, 0, 0, RST);
    } else {
      TransmissionControlProtocolSocket socket(this);
      socket.remotePort = msg->srcPort;
      socket.remoteIP = srcIP_BE;
      socket.localPort = msg->dstPort;
      socket.localIP = dstIP_BE;
      socket.sequenceNumber = bigEndian32(msg->acknowledgementNumber);
      socket.acknowledgementNumber =
          bigEndian32(msg->acknowledgementNumber) + 1;

      Send(&socket, 0, 0, RST);
    }
  }

  if (socket != 0 && socket->state == CLOSED)
    for (int i = 0; i < numSockets; i++)
      if (sockets[i] == socket) {
        sockets[i] = sockets[--numSockets];
        MemoryManager::activeMemoryManager->free(socket);
        break;
      }

  return false; // UDP never sends back
}

TransmissionControlProtocolSocket *
TransmissionControlProtocolProvider::Connect(common::uint32_t ip,
                                             common::uint16_t port) {
  TransmissionControlProtocolSocket *socket =
      (TransmissionControlProtocolSocket *)MemoryManager::activeMemoryManager
          ->malloc(sizeof(TransmissionControlProtocolSocket));
  if (socket != 0) {
    new (socket) TransmissionControlProtocolSocket(this);

    socket->remotePort = port;
    socket->remoteIP = ip;
    socket->localPort = freePort++;
    socket->localIP = provider->GetIPAddress();

    socket->remotePort = ((socket->remotePort & 0xFF00) >> 8) |
                         ((socket->remotePort & 0x00FF) << 8);
    socket->localPort = ((socket->localPort & 0xFF00) >> 8) |
                        ((socket->localPort & 0x00FF) << 8);

    sockets[numSockets++] = socket;

    socket->state = SYN_SENT;

    socket->sequenceNumber = 0xbeefcafe;
    socket->acknowledgementNumber = 0;

    Send(socket, 0, 0, SYN);
    printf("SYN SENT already\n");
  }
  return socket;
}

void TransmissionControlProtocolProvider::Disconnect(
    TransmissionControlProtocolSocket *socket) {

  socket->state = FIN_WAIT1;
  Send(socket, 0, 0, FIN + ACK);
  socket->sequenceNumber++;
}

void TransmissionControlProtocolProvider::Send(
    TransmissionControlProtocolSocket *socket, common::uint8_t *data,
    common::uint16_t size, common::uint16_t flags) {
  common::uint32_t totalLength =
      size + sizeof(TransmissionControlProtocolHeader);
  uint16_t lengthInclPHDr =
      totalLength + sizeof(TransmissionControlProtocolPseudoHeader);

  common::uint8_t *buffer =
      (common::uint8_t *)MemoryManager::activeMemoryManager->malloc(
          lengthInclPHDr);

  TransmissionControlProtocolPseudoHeader *phdr =
      (TransmissionControlProtocolPseudoHeader *)buffer;
  TransmissionControlProtocolHeader *msg =
      (TransmissionControlProtocolHeader
           *)(buffer + sizeof(TransmissionControlProtocolPseudoHeader));
  common::uint8_t *buffer2 = buffer +
                             sizeof(TransmissionControlProtocolHeader) +
                             sizeof(TransmissionControlProtocolPseudoHeader);

  msg->headerSize32 = sizeof(TransmissionControlProtocolHeader) / 4;
  msg->srcPort = socket->localPort;
  msg->dstPort = socket->remotePort;
  msg->acknowledgementNumber = bigEndian32(socket->acknowledgementNumber);
  msg->sequenceNumber = bigEndian32(socket->sequenceNumber);
  msg->flags = flags;
  msg->reserved = 0;
  msg->windowSize = 0xFFFF;
  msg->urgentPtr = 0; // not using urgent ptr
  msg->options = ((flags & SYN) != 0) ? 0xB4050402 : 0;

  socket->sequenceNumber += size;

  for (int i = 0; i < size; i++)
    buffer2[i] = data[i];

  phdr->srcIP = socket->localIP;
  phdr->dstIP = socket->remoteIP;
  phdr->protocol = 0x0600;
  phdr->totalLength =
      ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8);

  msg->checksum = 0;
  msg->checksum = InternetProtocolProvider::Checksum((common::uint16_t *)buffer,
                                                     lengthInclPHDr);
  InternetProtocolHandler::Send(socket->remoteIP, (common::uint8_t *)msg,
                                totalLength);

  MemoryManager::activeMemoryManager->free(buffer);
}

TransmissionControlProtocolSocket *
TransmissionControlProtocolProvider::Listen(common::uint16_t port) {
  TransmissionControlProtocolSocket *socket =
      (TransmissionControlProtocolSocket *)MemoryManager::activeMemoryManager
          ->malloc(sizeof(TransmissionControlProtocolSocket));
  if (socket != 0) {
    new (socket) TransmissionControlProtocolSocket(this);

    socket->state = LISTENING;
    socket->localIP = provider->GetIPAddress();
    socket->localPort = ((port & 0xFF00) >> 8) | ((port & 0x00FF) << 8);

    sockets[numSockets++] = socket;
  }
  return socket;
}

void TransmissionControlProtocolProvider::Bind(
    TransmissionControlProtocolSocket *socket,
    TransmissionControlProtocolHandler *handler) {
  socket->handler = handler;
}
} // namespace net
} // namespace myos
