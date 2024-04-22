
#include "wispnet.hpp"
#include "wispServer.hpp"
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

void init_wispnet() {

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  remove(WISPNET_SOCKET);
  strcpy(addr.sun_path, WISPNET_SOCKET);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("Could not start wispnet server %i\n", errno);
    exit(-1);
  }

  listen(fd, 10);
  std::thread(watch_wispnet, fd).detach();
}

void watch_wispnet(int fd) {
  while (true) {
    int client = accept(fd, NULL, NULL);
    std::thread(watch_wispnet_thread, client).detach();
  }
}
std::vector<WispNetPort> openPorts;
std::mutex portLock;

void watch_wispnet_thread(int client) {
  char largeBuffer[1024];
  size_t bufSize = 1024;
  void *deviceId = NULL;

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
    case 0xFF: {
      deviceId = *(void **)((char *)largeBuffer + sizeof(uint8_t));
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
    printf("WispNet init expected target id to have a value\n");
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
    }
  }
  portLock.unlock();
  if (!found) {
    return;
  }

  size_t dataSize = PACKET_SIZE(sizeof(uint32_t) + sizeof(uint32_t) +
                                sizeof(uint16_t) + size);
  void *dataPacket = calloc(1, dataSize);

  sendCallback(dataPacket, dataSize, targetId, false);
}
