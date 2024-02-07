#include "wispServer.hpp"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <byteswap.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
#include <vector>

std::map<uint32_t, SocketReference> socketManager;

void open_socket(ConnectPayload *payload, uint32_t streamId, SEND_CALLBACK_TYPE,
                 void *id) {
  SocketReference reference = {};

  reference.streamId = streamId;
  reference.id = id;
  reference.type = payload->type;

  std::cout << "Connection for: " << payload->hostname << "\n";

  int type = SOCK_STREAM;
  if (payload->type == 0x02) {
    type = SOCK_DGRAM;
  }

  reference.descriptor = socket(AF_INET, type, 0);
  if (reference.descriptor < 0) {
    set_exit_packet(sendCallback, id, streamId, ERROR_UNKNOWN);
    return;
  }

  struct hostent *dns = gethostbyname(payload->hostname);
  struct sockaddr *addrOut = (struct sockaddr *)malloc(
      sizeof(struct sockaddr_in) + sizeof(struct sockaddr_in6));

  if (dns == NULL) { // IP addr
    struct sockaddr_in *address = (struct sockaddr_in *)addrOut;

    address->sin_family = AF_INET;
    address->sin_port = htons(payload->port);

    if (inet_pton(AF_INET, payload->hostname, address) <= 0 &&
        inet_pton(AF_INET6, payload->hostname, address) <= 0) {
      set_exit_packet(sendCallback, id, streamId, ERROR_INVALID_CONNECTION);
      return;
    }
  } else { // domain
    if (dns->h_addrtype == AF_INET) {
      struct sockaddr_in *address = (struct sockaddr_in *)addrOut;

      address->sin_family = AF_INET;
      address->sin_port = htons(payload->port);
      address->sin_addr = *((struct in_addr *)(dns->h_addr_list[0]));
    } else { // IPV6
      struct sockaddr_in6 *address = (struct sockaddr_in6 *)addrOut;

      address->sin6_family = AF_INET6;
      address->sin6_port = htons(payload->port);
      address->sin6_addr = *((struct in6_addr *)(dns->h_addr_list[0]));
    }
  }
  if (payload->type == TCP_TYPE && // udp doesnt connect
      connect(reference.descriptor, addrOut, sizeof(struct sockaddr)) < 0) {
    switch (errno) {
    case ETIMEDOUT:
      set_exit_packet(sendCallback, id, streamId, ERROR_CONNECTION_TIMEOUT);
      break;
    case ECONNREFUSED:
      set_exit_packet(sendCallback, id, streamId, ERROR_REFUSED);
      break;
    case ENETUNREACH:
      set_exit_packet(sendCallback, id, streamId, ERROR_UNREACHABLE);
      break;
    default:
      set_exit_packet(sendCallback, id, streamId, ERROR_INVALID_CONNECTION);
    }
    return;
  }
  reference.addr = addrOut;

  socketManager[streamId] = reference;

  std::thread watch(watch_thread, streamId, sendCallback);
  watch.detach();
}
void watch_thread(uint32_t streamId, SEND_CALLBACK_TYPE) {
  for (auto id : socketManager) {
    if (id.first == streamId) {
      char buffer[READ_SIZE];
      ssize_t size;
      if (id.second.type == TCP_TYPE) {
        while ((size = recv(id.second.descriptor, buffer, READ_SIZE, 0)) > 0) {

          set_data_packet(buffer, size, id.first, sendCallback, id.second.id);
        }
      } else { // udp
        socklen_t addrSize = sizeof(struct sockaddr);
        while ((size = recvfrom(id.second.descriptor, buffer, READ_SIZE, 0,
                                id.second.addr, &addrSize)) > 0) {
          buffer[size] = '\0';
          set_data_packet(buffer, size, id.first, sendCallback, id.second.id);
        }
      }
      if (size != 0) {
        switch (errno) {
        case ECONNREFUSED:
          set_exit_packet(sendCallback, id.second.id, streamId, ERROR_REFUSED);
        default:
          set_exit_packet(sendCallback, id.second.id, streamId, ERROR_UNKNOWN);
        }
      }
      set_exit_packet(sendCallback, id.second.id, streamId, ERROR_UNKNOWN);
      return;
    }
  }
}

void set_exit_packet(SEND_CALLBACK_TYPE, void *id, uint32_t streamId,
                     char signal) {

  if (streamId != 0) {
    socketManager.erase(streamId);
  }
  size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  struct WispPacket *initPacket = (struct WispPacket *)std::calloc(1, initSize);
  initPacket->type = EXIT_PACKET;
  *(uint8_t *)((char *)&initPacket->payload - 3) = signal;
  *(uint32_t *)(&initPacket->type + sizeof(uint8_t)) = streamId;

  sendCallback(initPacket, initSize - 3, id, true);
}
void set_continue_packet(uint32_t bufferRemaining, SEND_CALLBACK_TYPE, void *id,
                         uint32_t streamId) {

  size_t continueSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  struct WispPacket *continuePacket =
      (struct WispPacket *)std::calloc(1, continueSize);
  continuePacket->type = CONTINUE_PACKET;
  *(uint32_t *)(&continuePacket->payload) = bufferRemaining;
  *(uint32_t *)(&continuePacket->type + sizeof(uint8_t)) = streamId;

  sendCallback(continuePacket, continueSize, id, false);
}
void set_data_packet(char *data, size_t size, uint32_t streamId,
                     SEND_CALLBACK_TYPE, void *id) {
  size_t dataSize = PACKET_SIZE(size);
  struct WispPacket *dataPacket = (struct WispPacket *)std::calloc(1, dataSize);
  dataPacket->type = DATA_PACKET;
  memcpy((char *)&dataPacket->payload - 3, data, size);
  *(uint32_t *)(&dataPacket->type + sizeof(uint8_t)) = streamId;

  printf("hey\n");

  sendCallback(dataPacket, dataSize, id, false);
}
void forward_data_packet(uint32_t streamId, SEND_CALLBACK_TYPE, void *id,
                         char *data, size_t length) {
  for (auto id : socketManager) {
    if (id.first == streamId) {
      ssize_t error;
      if (id.second.type == TCP_TYPE) {
        error = send(id.second.descriptor, data, length, 0);
      } else {
        error = sendto(id.second.descriptor, data, length, 0, id.second.addr,
                       sizeof(struct sockaddr));
      }
      if (error == -1) {
        switch (errno) {
        case ECONNRESET:
          set_exit_packet(sendCallback, id.second.id, streamId,
                          ERROR_NETWORK_ERROR);
          break;
        default:
          set_exit_packet(sendCallback, id.second.id, streamId, ERROR_UNKNOWN);
        }
        return;
      }
      if (id.second.type == TCP_TYPE) {
        set_continue_packet(BUFFER_SIZE, sendCallback, id.second.id, streamId);
      }
    }
  }
}
