#include "protocolExtensions.hpp"
#include "wispValidation.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

std::optional<ExtensionData> getDataForExtension(uint8_t id) {
  switch (id) {
  case PROTO_EXTENSION_UDP: {
    void *data = (0);
    ExtensionData ret;
    ret.payload = (char *)data;
    ret.size = 0;
    ret.id = id;
    return ret;
  }
  default:
    return {};
  }
}

void send_info(SEND_CALLBACK_TYPE, void *id) {
  size_t totalPayloadSize = sizeof(uint8_t) + sizeof(uint8_t);

  std::vector<ExtensionData> allExtensions;
  for (auto extension : suportedExtensions) {
    auto dataForExt = getDataForExtension(extension);
    if (dataForExt.has_value()) {
      allExtensions.push_back(dataForExt.value());
      totalPayloadSize += sizeof(uint8_t) + sizeof(uint8_t) + dataForExt->size;
    }
  }

  size_t dataSize = PACKET_SIZE(totalPayloadSize);

  void *dataPacket = calloc(1, dataSize);
  *(uint8_t *)dataPacket = WISP_INFO;
  *(uint32_t *)((char *)dataPacket + sizeof(uint8_t)) = 0;

  *(uint8_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t)) =
      MAJOR_VERSION;

  *(uint8_t *)((char *)dataPacket + sizeof(uint8_t) + sizeof(uint32_t) +
               sizeof(uint8_t)) = MINOR_VERSION;

  size_t cursor =
      sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t);
  for (auto extension : allExtensions) {
    *(uint8_t *)((char *)dataPacket + cursor) = extension.id;
    cursor += sizeof(uint8_t);

    *(uint8_t *)((char *)dataPacket + cursor) = extension.size;
    cursor += sizeof(uint8_t);

    memcpy((char *)dataPacket + cursor, extension.payload, extension.size);
    cursor += extension.size;
  }
  if (cursor > dataSize) {
    printf("Buffer overflow !!!!\n");
  }

  sendCallback(dataPacket, dataSize, id, false);
}

/* EX PARSE
 * 0500 0000 0002 0101 0002 0b08 0001 7573
 * 6572 6e61 6d65 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6161 6161 6161 6161 6161 6161
 * 6161 6161 6162
 */
