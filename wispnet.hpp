#pragma once
#include "wispServer.hpp"
#include <cstdint>

const char WISPNET_SOCKET[] = "/tmp/wispnet";
void init_wispnet();
void watch_wispnet(int sock);
void watch_wispnet_thread(int client);

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
struct WispNetStream {
  void *serverId;
  uint16_t port;
  void *clientId;
  uint32_t connectionId;
  uint8_t type;
};
