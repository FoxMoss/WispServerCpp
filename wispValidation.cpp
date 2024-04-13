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
  case WISP_INFO:
    if (size < PACKET_SIZE((size_t)sizeof(uint8_t) + sizeof(uint8_t)))
      return WISP_NULL;
    return WISP_INFO;
  default:
    return WISP_NULL;
  }
  return WISP_NULL;
}

std::map<void *, unsigned int> forwardsRequested;
std::mutex ratelimitLock;

// true == limit
bool checkRatelimits(void *ip) {
  if (maxForward == -1) {
    return false;
  }
  ratelimitLock.lock();
  if (forwardsRequested.find(ip) == forwardsRequested.end()) {
    forwardsRequested[ip] = 0;
  }
  forwardsRequested[ip]++;
#ifdef DEBUG
  printf("This minute on %p now at %i forwards\n", ip, forwardsRequested[ip]);
#endif // DEBUG

  std::thread([ip]() {
    std::this_thread::sleep_for(60s);
    ratelimitLock.lock();
    forwardsRequested[ip]--;
#ifdef DEBUG
    printf("This minute on %p now at %i forwards\n", ip, forwardsRequested[ip]);
#endif // DEBUG
    if (forwardsRequested[ip] <= 0) {
      forwardsRequested.erase(ip);
    }
    ratelimitLock.unlock();
  }).detach();

  ratelimitLock.unlock();
  return forwardsRequested[ip] > maxForward;
}
