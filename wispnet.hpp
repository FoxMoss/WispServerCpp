#pragma once
#include "wispServer.hpp"
#include <cstdint>

const char WISPNET_SOCKET[] = "/tmp/wispnet";
void init_wispnet(SEND_CALLBACK_TYPE);
void watch_wispnet(int sock, SEND_CALLBACK_TYPE);
void watch_wispnet_thread(int client, SEND_CALLBACK_TYPE);

void send_wispnet_init(void *targetId, uint32_t streamId, SEND_CALLBACK_TYPE,
                       int fd);
void send_wispnet_data(void *targetId, SEND_CALLBACK_TYPE, void *fromId,
                       uint32_t connectionId, uint16_t port, char *data,
                       size_t size);
void send_wispnet_connect(void *targetId, SEND_CALLBACK_TYPE, void *fromId,
                          uint32_t connectionId, uint8_t type, uint16_t port);
void send_wispnet_exit(void *targetId, uint32_t connectionId, uint16_t port);
void send_wispnet_registry(int fd);

enum WISPNET_CLIENT_PROTO_TYPES {
  WNC_PROBE = 0x01,
  WNC_OPEN = 0x02,
  WNC_SERVER_DATA = 0x03,
  WNC_SERVER_EXIT = 0x04,
};
enum WISPNET_SERVER_PROTO_TYPES {
  WNS_REGISTERED = 0x01,
  WNS_REGISTRY = 0x02,
  WNS_FWD_CONNECT = 0x03,
  WNS_FWD_DATA = 0x04,
};

struct WispNetPort {
  void *deviceId;
  uint16_t port;
  bool discoverable;
  char *note;
  size_t noteSize;
  int fd;
};
