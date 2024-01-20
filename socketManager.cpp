#include "wispServer.hpp"
#include <arpa/inet.h>
#include <byteswap.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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

void open_socket(struct ConnectPayload *payload, uint32_t streamId, Server *s,
                 websocketpp::connection_hdl hdl) {
  SocketReference reference = {};

  reference.streamId = streamId;
  reference.hdl = hdl;
  reference.s = s;
  reference.type = payload->type;

  int type = SOCK_STREAM;
  if (payload->type == 0x02) {
    type = SOCK_DGRAM;
  }

  reference.descriptor = socket(AF_INET, type, 0);
  if (reference.descriptor < 0) {
    set_exit_packet(s, hdl, streamId);
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
      set_exit_packet(s, hdl, streamId);
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
  if (connect(reference.descriptor, addrOut, sizeof(struct sockaddr)) < 0) {
    set_exit_packet(s, hdl, streamId);
    return;
  }

  socketManager[streamId] = reference;

  std::thread watch(watch_thread, streamId);
  watch.detach();
}
void watch_thread(uint32_t streamId) {
  for (auto id : socketManager) {
    if (id.first == streamId) {
      char buffer[2048];
      size_t size;
      while ((size = recv(id.second.descriptor, buffer, 2048, 0))) {
        set_data_packet(buffer, size, id.first, id.second.s, id.second.hdl);
      }
      set_exit_packet(id.second.s, id.second.hdl, id.first, 2);
      return;
    }
  }
}

void set_exit_packet(Server *s, websocketpp::connection_hdl hdl,
                     uint32_t streamId, char signal) {

  if (streamId != 0) {
    socketManager.erase(streamId);
  }
  if (!hdl.expired()) {
    size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
    struct WispPacket *initPacket =
        (struct WispPacket *)std::calloc(1, initSize);
    initPacket->type = EXIT_PACKET;
    *(uint8_t *)((char *)&initPacket->payload - 3) = signal;
    *(uint32_t *)(&initPacket->type + sizeof(uint8_t)) = streamId;

    s->send(hdl, initPacket, initSize - 3, websocketpp::frame::opcode::BINARY);

    s->close(hdl, websocketpp::close::status::normal, "");
  }
}
void set_continue_packet(uint32_t bufferRemaining, Server *s,
                         websocketpp::connection_hdl hdl, uint32_t streamId) {
  if (!hdl.expired()) {

    size_t continueSize = PACKET_SIZE((size_t)sizeof(uint32_t));
    struct WispPacket *continuePacket =
        (struct WispPacket *)std::calloc(1, continueSize);
    continuePacket->type = CONTINUE_PACKET;
    *(uint32_t *)(&continuePacket->payload) = bufferRemaining;
    *(uint32_t *)(&continuePacket->type + sizeof(uint8_t)) = streamId;

    s->send(hdl, continuePacket, continueSize,
            websocketpp::frame::opcode::BINARY);
  }
}
void set_data_packet(char *data, size_t size, uint32_t streamId, Server *s,
                     websocketpp::connection_hdl hdl) {
  if (!hdl.expired()) {
    size_t dataSize = PACKET_SIZE(size);
    struct WispPacket *dataPacket =
        (struct WispPacket *)std::calloc(1, dataSize);
    dataPacket->type = DATA_PACKET;
    memcpy((char *)&dataPacket->payload - 3, data, size);
    *(uint32_t *)(&dataPacket->type + sizeof(uint8_t)) = streamId;

    s->send(hdl, dataPacket, dataSize, websocketpp::frame::opcode::BINARY);
  }
}
void forward_data_packet(uint32_t streamId, Server *s,
                         websocketpp::connection_hdl hdl, char *data,
                         size_t length) {
  for (auto id : socketManager) {
    if (id.first == streamId) {
      std::cout << length << "\n";
      send(id.second.descriptor, data, length, 0);
      if (id.second.type == TCP_TYPE) {
        set_continue_packet(BUFFER_SIZE, s, hdl, streamId);
      }
    }
  }
}
