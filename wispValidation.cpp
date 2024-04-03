#include "wispValidation.hpp"
#include "wispServer.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

WISP_PACKET_TYPE validatePacket(char *buffer, size_t size) {
  if (size < PACKET_SIZE((size_t)sizeof(uint8_t)))
    return WISP_NULL;
  switch (buffer[0]) {
  case WISP_CONNECT: {
    if (size < PACKET_SIZE(WISP_PAYLOAD_CONNECT_SIZE))
      return WISP_NULL;
    uint8_t type = *(uint8_t *)(buffer + PACKET_SIZE(0));
    if (type != UDP_TYPE && type != TCP_TYPE)
      return WISP_NULL;
    return WISP_CONNECT;
  }
  case WISP_DATA:
    if (size < PACKET_SIZE((size_t)sizeof(char)))
      return WISP_NULL;
    return WISP_DATA;
  case WISP_CONTINUE: {
    if (size != PACKET_SIZE((size_t)sizeof(uint32_t)))
      return WISP_NULL;
    return WISP_CONTINUE;
  }
  case WISP_CLOSE:
    if (size != PACKET_SIZE((size_t)sizeof(uint8_t)))
      return WISP_NULL;
    return WISP_CLOSE;
  default:
    return WISP_NULL;
  }
  return WISP_NULL;
}

std::map<std::string, unsigned int> openedConnections;
std::mutex ratelimitLock;

// true == limit
bool checkRatelimits(std::string ip, char *buffer) {
  if (maxConnect == -1) {
    return false;
  }
  // ratelimitLock.lock();
  if (openedConnections.find(ip) == openedConnections.end()) {
    openedConnections[ip] = 0;
  }
  if (buffer[0] != WISP_CONNECT) {
    return openedConnections[ip] > maxConnect;
  }
  openedConnections[ip]++;

  std::thread([ip]() {
    std::this_thread::sleep_for(60s);
    // ratelimitLock.lock();
    openedConnections[ip]--;
    if (openedConnections[ip] <= 0) {
      openedConnections.erase(ip);
    }
    // ratelimitLock.unlock();
  }).detach();

  // ratelimitLock.unlock();
  return openedConnections[ip] > maxConnect;
}
