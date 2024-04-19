#include "wispServer.hpp"
#include "wispValidation.hpp"
#include "wispnet.hpp"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <byteswap.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

void open_socket(ConnectPayload *payload, uint32_t streamId, SEND_CALLBACK_TYPE,
                 void *id) {
  SocketReference reference = {};

  reference.streamId = streamId;
  reference.id = id;
  reference.type = payload->type;

  int type = SOCK_STREAM;
  if (payload->type == 0x02) {
    type = SOCK_DGRAM;
  }

  struct sockaddr *addrOut = (struct sockaddr *)malloc(
      sizeof(struct sockaddr_in) + sizeof(struct sockaddr_in6) +
      sizeof(sockaddr_un));

  struct hostent *dns = gethostbyname(payload->hostname);

  if (strcmp(payload->hostname, "wispnet") == 0) {
    reference.wispNet = true;

    struct sockaddr_un *addr = (struct sockaddr_un *)addrOut;
    reference.descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, "/tmp/wispnet");

    if (connect(reference.descriptor, addrOut, sizeof(sockaddr_un)) < 0) {
      set_exit_packet(sendCallback, id, streamId, ERROR_UNKNOWN);
      return;
    }

    char *sendIdData = (char *)malloc(sizeof(uint8_t) + sizeof(void *));
    *(uint8_t *)(sendIdData) = 0xFF;
    *(void **)(sendIdData + sizeof(uint8_t)) = id;
    send(reference.descriptor, sendIdData, sizeof(uint8_t) + sizeof(void *), 0);
    free(sendIdData);

    goto skipNormalCreation;
  }

  reference.descriptor = socket(AF_INET, type, 0);
  if (reference.descriptor < 0) {
    set_exit_packet(sendCallback, id, streamId, ERROR_UNKNOWN);
    return;
  }

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
  if (payload->type == TCP_TYPE && // udp doesnt use connect
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
skipNormalCreation:

  socketGaurd.lock();
  socketManager.push_back(reference);
#ifdef DEBUG
  std::cout << "Connection for: " << payload->hostname << ":" << payload->port
            << " on id " << id << " : " << streamId << "\n";
#endif // DEBUG
  socketGaurd.unlock();

  std::thread watch(watch_thread, streamId, sendCallback, id);
  watch.detach();
}
void watch_thread(uint32_t streamId, SEND_CALLBACK_TYPE, void *id) {
  SocketReference copy;
  bool found = false;
  socketGaurd.lock();
  for (auto find = socketManager.begin();
       find != socketManager.end() && socketManager.size() != 0; find++) {
    if (find->streamId == streamId && find->id == id) {
      found = true;
      copy = *find.base();
      break;
    }
  }
  socketGaurd.unlock();

  if (!found) {
#ifdef DEBUG
    std::cout << "Error in creating thread\n";
#endif // DEBUG
    return;
  }

  char buffer[READ_SIZE];
  ssize_t size;
  if (copy.type == TCP_TYPE) {
    while ((size = recv(copy.descriptor, buffer, READ_SIZE, 0)) > 0) {

      set_data_packet(buffer, size, copy.streamId, sendCallback, copy.id);
    }
  } else { // udp
    socklen_t addrSize = sizeof(struct sockaddr);
    while ((size = recvfrom(copy.descriptor, buffer, READ_SIZE, 0, copy.addr,
                            &addrSize)) > 0) {
      buffer[size] = '\0';
      set_data_packet(buffer, size, copy.streamId, sendCallback, copy.id);
    }
  }
  if (size == 0) {
    switch (errno) {
    case ECONNREFUSED:
      set_exit_packet(sendCallback, copy.id, streamId, ERROR_REFUSED);
    default:
      set_exit_packet(sendCallback, copy.id, streamId, ERROR_UNKNOWN);
    }
  } else {
    set_exit_packet(sendCallback, copy.id, streamId, ERROR_UNKNOWN);
  }
  socketGaurd.lock();
  free(copy.addr);
  std::vector<SocketReference>::iterator itCopy;

  for (auto find = socketManager.begin();
       find != socketManager.end() && socketManager.size() != 0; find++) {
    if (find->streamId == streamId && find->id == id) {
      socketManager.erase(find);
      break;
    }
  }
  socketGaurd.unlock();

  return;
}

void set_exit_packet(SEND_CALLBACK_TYPE, void *id, uint32_t streamId,
                     char signal) {

  size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  void *initPacket = calloc(1, initSize);
  *(uint8_t *)initPacket = WISP_CLOSE;
  *(uint8_t *)((char *)initPacket + sizeof(uint8_t) + sizeof(uint32_t)) =
      signal;
  *(uint32_t *)((char *)initPacket + sizeof(uint8_t)) = streamId;

#ifdef DEBUG
  std::cout << "Exited on id " << id << " : " << streamId
            << " for reason: " << (int)signal << "\n";
#endif

  sendCallback(initPacket, initSize - 3, id, false);
}

void set_continue_packet(uint32_t bufferRemaining, SEND_CALLBACK_TYPE, void *id,
                         uint32_t streamId) {

  size_t continueSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  void *continuePacket = (void *)calloc(1, continueSize);
  *(uint8_t *)continuePacket = WISP_CONTINUE;
  *(uint32_t *)((char *)continuePacket + sizeof(uint8_t) + sizeof(uint32_t)) =
      bufferRemaining;
  *(uint32_t *)((char *)continuePacket + sizeof(uint8_t)) = streamId;

  sendCallback(continuePacket, continueSize, id, false);
}
void set_data_packet(char *data, size_t size, uint32_t streamId,
                     SEND_CALLBACK_TYPE, void *id) {
  size_t dataSize = PACKET_SIZE(size);
  void *dataPacket = calloc(1, dataSize);
  *(uint8_t *)dataPacket = WISP_DATA;
  memcpy((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t), data, size);
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t)) = streamId;

  sendCallback(dataPacket, dataSize, id, false);
}
void forward_data_packet(uint32_t streamId, SEND_CALLBACK_TYPE, void *id,
                         char *data, size_t length) {
  if (checkRatelimits(id)) {
    set_exit_packet(sendCallback, id, streamId, ERROR_THROTTLED);
    return;
  }

  bool found = false;
  SocketReference idData;
  socketGaurd.lock();
  for (auto copy : socketManager) {
    if (copy.streamId == streamId && copy.id == id) {
      found = true;
      idData = copy;
    }
  }
  socketGaurd.unlock();
  if (!found) {
#ifdef DEBUG
    std::cout << "Matching socket not found. " << id << " : " << streamId
              << "\n";
#endif // DEBUG
    return;
  }

  ssize_t error;
  if (idData.type == TCP_TYPE) {
    error = send(idData.descriptor, data, length, 0);
  } else {
    error = sendto(idData.descriptor, data, length, 0, idData.addr,
                   sizeof(struct sockaddr));
  }
  if (error == -1) {
    switch (errno) {
    case ECONNRESET:
      set_exit_packet(sendCallback, idData.id, streamId, ERROR_NETWORK_ERROR);
      break;
    default:
      set_exit_packet(sendCallback, idData.id, streamId, ERROR_UNKNOWN);
    }
    return;
  }
  if (idData.type == TCP_TYPE) {
    set_continue_packet(BUFFER_SIZE, sendCallback, idData.id, streamId);
  }
}

void close_sockets(void *id) {
  socketGaurd.lock();
#ifdef DEBUG
  std::cout << "Closed sockets on id: " << id;
#endif // DEBUG

  // iterators caused issues tf???
  for (auto sock = 0; sock < socketManager.size(); sock++) {
    if (socketManager[sock].id == id) {
      close(socketManager[sock].descriptor);
      socketManager.erase(socketManager.begin() + sock);
    }
  }
  socketGaurd.unlock();
}
