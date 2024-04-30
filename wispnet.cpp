#include "wispnet.hpp"
#include "wispServer.hpp"
#include "wispValidation.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

void init_wispnet(SEND_CALLBACK_TYPE) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  remove(WISPNET_SOCKET);
  strcpy(addr.sun_path, WISPNET_SOCKET);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("Could not start wispnet server %i\n", errno);
    exit(-1);
  }

  listen(fd, -1);
  std::thread(watch_wispnet, fd, sendCallback).detach();
}

void watch_wispnet(int fd, SEND_CALLBACK_TYPE) {
  while (true) {
    int client = accept(fd, NULL, NULL);
    std::thread(watch_wispnet_thread, client, sendCallback).detach();
  }
}
std::vector<WispNetPort> openPorts;
std::mutex portLock;

void watch_wispnet_thread(int client, SEND_CALLBACK_TYPE) {
  char largeBuffer[1024];
  void *deviceId = NULL;
  uint32_t streamId = 0;

  while (true) {
    ssize_t bufferAddition = recv(client, largeBuffer, 1024, 0);
    if (bufferAddition >= 1024) {
      printf("Fix wispnet code to be better\n");
    }
    if (bufferAddition == -1) {
      break;
    }

    switch (*(uint8_t *)(largeBuffer)) {
    case WNC_OPEN: {
      if (deviceId == NULL) {
        printf("Invalid unix socket\n");
        return;
      }

      WispNetPort portObject;

      portObject.port = *(uint16_t *)((char *)largeBuffer + sizeof(uint8_t));
      portObject.discoverable =
          *(bool *)((char *)largeBuffer + sizeof(uint8_t) + sizeof(uint16_t));

      size_t cursor = 0;
      for (; cursor < bufferAddition -
                          (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(bool));
           cursor++) {
        if (largeBuffer[cursor + sizeof(uint8_t) + sizeof(uint16_t) +
                        sizeof(bool)] == '\0')
          break;
      }
      ++cursor; // null term

      portObject.note = (char *)malloc(cursor);
      portObject.fd = client;

      memcpy(portObject.note,
             (char *)&largeBuffer + sizeof(uint8_t) + sizeof(uint16_t) +
                 sizeof(bool),
             cursor);

      portObject.deviceId = deviceId;
      portLock.lock();
      openPorts.push_back(portObject);
      portLock.unlock();
      break;
    }
    case WNC_SERVER_DATA: {
      uint32_t clientId = *(uint32_t *)((char *)largeBuffer + sizeof(uint8_t));
      uint32_t connectionId = *(uint32_t *)((char *)largeBuffer +
                                            sizeof(uint8_t) + sizeof(uint32_t));
      uint16_t port = *(uint16_t *)((char *)largeBuffer + sizeof(uint8_t) +
                                    sizeof(uint32_t) + sizeof(uint32_t));
      size_t dataSize = bufferAddition - (sizeof(uint8_t) + sizeof(uint32_t) +
                                          sizeof(uint32_t) + sizeof(uint16_t));

      auto clientPtr = wsMap.find(clientId);

      if (!clientPtr.has_value()) {
        send_wispnet_exit(deviceId, connectionId, port);
        break;
      }

      bool found = false;
      uint32_t streamId = 0;
      socketGaurd.lock();
      for (auto index = socketManager.begin(); index != socketManager.end();
           index++) {
        if (index->id == clientPtr && index->connectionId == connectionId &&
            index->port == port && index->targetId == deviceId) {
          found = true;
          streamId = index->streamId;
          break;
        }
      }
      socketGaurd.unlock();

      if (!clientPtr.has_value() || !found) {
        send_wispnet_exit(deviceId, connectionId, port);
        break;
      }

      set_data_packet((char *)largeBuffer + sizeof(uint8_t) + sizeof(uint32_t) +
                          sizeof(uint32_t) + sizeof(uint16_t),
                      dataSize, streamId, sendCallback, clientPtr.value());
      break;
    }
    case WNC_SERVER_EXIT: {
      uint32_t clientId = *(uint32_t *)((char *)largeBuffer + sizeof(uint8_t));
      uint32_t connectionId = *(uint32_t *)((char *)largeBuffer +
                                            sizeof(uint8_t) + sizeof(uint32_t));
      uint16_t port = *(uint16_t *)((char *)largeBuffer + sizeof(uint8_t) +
                                    sizeof(uint32_t) + sizeof(uint32_t));

      auto clientPtr = wsMap.find(clientId);

      if (!clientPtr.has_value()) {
        send_wispnet_exit(deviceId, connectionId, port);
        break;
      }

      bool found = false;
      uint32_t streamId = 0;
      socketGaurd.lock();
      for (auto index = socketManager.begin(); index != socketManager.end();
           index++) {
        if (index->id == clientPtr && index->connectionId == connectionId &&
            index->port == port && index->targetId == deviceId) {
          found = true;
          streamId = index->streamId;
          break;
        }
      }
      socketGaurd.unlock();

      if (!clientPtr.has_value() || !found) {
        send_wispnet_exit(deviceId, connectionId, port);
        break;
      }

      set_exit_packet(sendCallback, clientPtr.value(), streamId);
      break;
    }
    case 0xFF: {
      if (deviceId != 0) {
        break;
      }
      deviceId = *(void **)((char *)largeBuffer + sizeof(uint8_t));
      streamId =
          *(uint32_t *)((char *)largeBuffer + sizeof(uint8_t) + sizeof(void *));
      socketGaurd.lock();
      for (auto index = socketManager.begin(); index != socketManager.end();
           index++) {
        if (index->id == deviceId && index->streamId == streamId) {
          index->serverFd = client;
          break;
        }
      }
      socketGaurd.unlock();
      send_wispnet_init(deviceId, client);
    }
    default:
      break;
    }
  }
  portLock.lock();
  for (auto portIterator = openPorts.begin(); portIterator != openPorts.end();
       portIterator++) {
    if (portIterator->deviceId == deviceId) {
      free(portIterator->note);
      openPorts.erase(portIterator);
    }
  }
  portLock.unlock();
  close(client);
}
void send_wispnet_init(void *targetId, int fd) {
  const size_t size = sizeof(uint8_t) + sizeof(uint32_t);
  char data[size];
  data[0] = 0x01;
  if (!wsMap[targetId].has_value()) {
#ifdef DEBUG
    printf("WispNet init expected target id to have a value\n");
#endif // DEBUG
    exit(-1);
  }
  *(uint32_t *)((char *)data + sizeof(uint8_t)) = wsMap[targetId].value();
  send(fd, data, size, 0);
}

void send_wispnet_data(void *targetId, SEND_CALLBACK_TYPE, void *fromId,
                       uint32_t connectionId, uint16_t port, char *data,
                       size_t size) {
  bool found = false;
  WispNetPort portInfo;
  portLock.lock();
  for (auto portDevice : openPorts) {
    if (portDevice.deviceId == targetId && portDevice.port == port) {
      found = true;
      portInfo = portDevice;
      break;
    }
  }
  portLock.unlock();
  if (!found) {
    return;
  }

  const size_t dataSize = sizeof(uint8_t) + sizeof(uint32_t) +
                          sizeof(uint32_t) + sizeof(uint16_t) + size;
  char dataPacket[dataSize];

  *(uint8_t *)dataPacket = 0x04;
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t)) = wsMap[fromId].value();
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t)) =
      connectionId;
  *(uint16_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t) +
                sizeof(uint32_t)) = port;
  memcpy((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t) +
             sizeof(uint32_t) + sizeof(uint16_t),
         data, size);

  send(portInfo.fd, dataPacket, dataSize, 0);
}
void send_wispnet_connect(void *targetId, SEND_CALLBACK_TYPE, void *fromId,
                          uint32_t connectionId, uint8_t type, uint16_t port) {
  bool found = false;
  WispNetPort portInfo;
  portLock.lock();
  for (auto portDevice : openPorts) {
    if (portDevice.deviceId == targetId && portDevice.port == port) {
      found = true;
      portInfo = portDevice;
      break;
    }
  }
  portLock.unlock();
  if (!found) {
    return;
  }

  const size_t dataSize = sizeof(uint8_t) + sizeof(uint32_t) +
                          sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);
  char dataPacket[dataSize];

  *(uint8_t *)dataPacket = 0x03;
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t)) = wsMap[fromId].value();
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t)) =
      connectionId;
  *(uint8_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t) +
               sizeof(uint32_t)) = type;
  *(uint16_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t) +
                sizeof(uint32_t) + sizeof(uint8_t)) = port;

  send(portInfo.fd, dataPacket, dataSize, 0);
}
void send_wispnet_exit(void *targetId, uint32_t connectionId, uint16_t port) {
  printf("WispNet exit is to be implemented. \n");
}
