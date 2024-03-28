#pragma once

#include <cstddef>

enum WISP_PACKET_TYPE {
  WISP_NULL = 0xff,
  WISP_CONNECT = 0x01,
  WISP_DATA = 0x02,
  WISP_CONTINUE = 0x03,
  WISP_CLOSE = 0x04
};

#define WISP_PAYLOAD_CONNECT_SIZE                                              \
  (size_t)(sizeof(uint8_t) + sizeof(uint16_t) + sizeof(char))

WISP_PACKET_TYPE validatePacket(char *buffer, size_t size);
