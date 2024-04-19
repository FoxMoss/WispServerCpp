
#include "wispnet.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
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
std::vector<WispNetPort>
    openPorts; // TODO: free notes otherwise infinite memory glitch
void watch_wispnet_thread(int client) {
  char largeBuffer[1024];
  size_t bufSize = 1024;
  void *deviceId = NULL;

  while (true) {
    ssize_t bufferAddition = recv(client, largeBuffer, 1024, 0);
    if (bufferAddition >= 1024) {
      printf("Fix wispnet code to be better\n");
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
      openPorts.push_back(portObject);
      break;
    }
    case 0xFF: {
      deviceId = *(void **)((char *)largeBuffer + sizeof(uint8_t));
    }
    default:
      break;
    }
  }
  close(client);
}
