#pragma once
#include "wispServer.hpp"
#include "wispValidation.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct ExtensionData {
  char *payload;
  size_t size;
  uint8_t id;
};

void send_info(SEND_CALLBACK_TYPE, void *id);
void parse_info_packet(uint32_t streamId, SEND_CALLBACK_TYPE, void *id,
                       char *data, size_t length);

const uint8_t PROTO_EXTENSION_UDP = 0x01;
const uint8_t PROTO_EXTENSION_PASSWORD_AUTH = 0x02;

std::optional<ExtensionData> getDataForExtension(uint8_t id);
const std::vector<uint8_t> suportedExtensions = {PROTO_EXTENSION_UDP};
