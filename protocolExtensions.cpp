#include "protocolExtensions.hpp"
#include "wispServer.hpp"
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
      totalPayloadSize += sizeof(uint8_t) + sizeof(uint32_t) + dataForExt->size;
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

    *(uint32_t *)((char *)dataPacket + cursor) = extension.size;
    cursor += sizeof(uint32_t);

    memcpy((char *)dataPacket + cursor, extension.payload, extension.size);
    cursor += extension.size;
  }
  if (cursor > dataSize) {
    printf("Buffer overflow !!!!\n");
  }

  sendCallback(dataPacket, dataSize, id, false);
}

void parse_info_packet(uint32_t streamId, SEND_CALLBACK_TYPE, void *id,
                       char *data, size_t length) {

  uint8_t clientMajorVersion = *(uint8_t *)((char *)data);
  if (clientMajorVersion > MAJOR_VERSION || clientMajorVersion < 1) {
    set_exit_packet(sendCallback, id, 0, ERROR_INCOMPATABLE);
    return;
  }
  uint8_t clientMinorVersion = *(uint8_t *)((char *)data + sizeof(uint8_t));

  size_t cursor = sizeof(uint8_t) * 2;
  while (cursor < length) {
    uint8_t extensionId = *(uint8_t *)((char *)data + cursor);
    cursor += sizeof(uint8_t);

    uint32_t size = *(uint32_t *)((char *)data + cursor);
    cursor += sizeof(uint32_t);

    bool found = false;
    for (auto extensionIndex : suportedExtensions) {
      if (extensionIndex == extensionId) {
        found = true;
      }
    }
    if (!found) {
      set_exit_packet(sendCallback, id, 0, ERROR_INCOMPATABLE);
#ifdef DEBUG
      printf("Client %p try to access proto extension %i\n", id, extensionId);
#endif // DEBUG
      return;
    }

    switch (extensionId) {
    case PROTO_EXTENSION_UDP: {
      break;
    }
    default: {
      set_exit_packet(sendCallback, id, 0, ERROR_INCOMPATABLE);
#ifdef DEBUG
      printf("Client %p try to access proto extension %i\n", id, extensionId);
#endif // DEBUG
      return;
    }
    }

    cursor += size;
  }
}
