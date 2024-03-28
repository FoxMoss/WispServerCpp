#include "wispValidation.hpp"
#include "wispServer.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>

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
