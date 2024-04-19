#pragma once

#include "interface.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

#ifdef DEBUG
// inline std::map<void *, bool> TBF;
//
// #define malloc(arg1) \
//   [=]() -> void * { \
//     void *ptr = malloc(arg1); \
//     printf("malloc %p thread %lu size %lu\n", ptr, pthread_self(), arg1); \
//     TBF[ptr] = true; \
//     return ptr; \
//   }()
//
// #define calloc(a, arg1) \
//   [=]() -> void * { \
//     void *ptr = calloc(a, arg1); \
//     printf("malloc %p thread %lu size %lu\n", ptr, pthread_self(), arg1); \
//     TBF[ptr] = true; \
//     return ptr; \
//   }()
//
// #define free(ptr) \
//   printf("free %p thread %lu first byte %i\n", ptr, pthread_self(), \
//          ((char *)ptr)[0]); \
//   TBF.erase(nullptr); \
//   printf("wasting away %p\n", TBF.end()->first); \ free(ptr);
#endif // DEBUG

#define BUFFER_SIZE 1024
#define READ_SIZE 1024

#define TCP_TYPE 0x01
#define UDP_TYPE 0x02

#define ERROR_UNKNOWN 0x01
#define ERROR_RESET 0x02
#define ERROR_NETWORK_ERROR 0x03
#define ERROR_INCOMPATABLE 0x04
#define ERROR_INVALID_CONNECTION 0x41
#define ERROR_UNREACHABLE 0x42
#define ERROR_CONNECTION_TIMEOUT 0x43
#define ERROR_REFUSED 0x44
#define ERROR_DATA_TIMEOUT 0x47
#define ERROR_BLOCKED 0x48
#define ERROR_THROTTLED 0x49
#define ERROR_CLIENT 0x89

#define PACKET_SIZE(payload_size)                                              \
  sizeof(uint8_t) + sizeof(uint32_t) + (sizeof(char) * (payload_size))

struct ConnectPayload {
  uint8_t type; // 0x01 == tcp 0x02 == udp
  uint16_t port;
  char hostname[];
};

struct WispPacket {
  uint8_t type;
  uint32_t streamId;
  char payload[];
};

struct SocketReference {
  int descriptor;
  int64_t streamId;
  uint8_t type; // 0x01 == tcp 0x02 == udp
  void *id;
  struct sockaddr *addr;
  bool wispNet = false;
};

void set_exit_packet(SEND_CALLBACK_TYPE, void *id, uint32_t streamId = 0,
                     char signal = 0x01);
void set_continue_packet(uint32_t bufferRemaining, SEND_CALLBACK_TYPE, void *id,
                         uint32_t streamId = 0);
void open_socket(ConnectPayload *payload, uint32_t streamId, SEND_CALLBACK_TYPE,
                 void *id);
void forward_data_packet(uint32_t streamId, SEND_CALLBACK_TYPE, void *id,
                         char *data, size_t length);
void set_data_packet(char *data, size_t size, uint32_t streamId,
                     SEND_CALLBACK_TYPE, void *id);
void watch_thread(uint32_t streamId, SEND_CALLBACK_TYPE, void *id);
void close_sockets(void *id);

inline std::vector<SocketReference> socketManager;
inline std::mutex socketGaurd;
