#pragma once
#include "wispServer.hpp"
#include <cstdint>

const char WISPNET_SOCKET[] = "/tmp/wispnet";
void init_wispnet();
void watch_wispnet(int sock);
void watch_wispnet_thread(int client);
void send_wispnet_init(void *targetId, int fd);
void send_wispnet_data(void *targetId, SEND_CALLBACK_TYPE, void *fromId,
                       uint32_t connectionId, uint16_t port, char *data,
                       size_t size);

enum WISPNET_CLIENT_PROTO_TYPES {
  WNC_PROBE = 0x01,
  WNC_OPEN = 0x02,
};

struct WispNetPort {
  void *deviceId;
  uint16_t port;
  bool discoverable;
  char *note;
};
