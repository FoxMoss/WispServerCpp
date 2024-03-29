#include "interface.hpp"
#include "wispServer.hpp"
#include <bits/types/error_t.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>

void message_interface(SEND_CALLBACK_TYPE, std::string msg, void *id) {

  size_t payloadLength =
      (size_t)msg.size() - sizeof(uint8_t) - sizeof(uint32_t);
  struct WispPacket *recvPacket = (struct WispPacket *)calloc(
      1, sizeof(char) * (msg.size() + 1) + sizeof(WispPacket));

  const char *data = msg.c_str();

  recvPacket->type = *(uint8_t *)data;
  recvPacket->streamId = *(uint32_t *)(data + sizeof(uint8_t));
  memcpy(&recvPacket->payload, data + sizeof(uint8_t) + sizeof(uint32_t),
         payloadLength);

  switch (recvPacket->type) {
  case 0x01: // Connect
  {
    struct ConnectPayload *payload =
        (struct ConnectPayload *)malloc(payloadLength + sizeof(ConnectPayload));
    payload->type = *(uint8_t *)(&recvPacket->payload);

    payload->port =
        *(uint16_t *)((char *)&recvPacket->payload + sizeof(uint8_t));

    if (payloadLength - sizeof(uint8_t) + sizeof(uint16_t) == 1) {
      return;
    }
    memcpy(&payload->hostname,
           (char *)((char *)&recvPacket->payload + sizeof(uint8_t) +
                    sizeof(uint16_t)),
           payloadLength - sizeof(uint8_t) - sizeof(uint16_t));

    payload->hostname[payloadLength - 3] = 0;

    open_socket(payload, recvPacket->streamId, sendCallback, id);

    free(payload);
  } break;
  case 0x02: {
    char *payloadRaw = (char *)malloc(payloadLength + 1);

    memcpy(payloadRaw, (char *)(&recvPacket->payload), payloadLength);

    forward_data_packet(recvPacket->streamId, sendCallback, id,
                        (char *)payloadRaw, payloadLength);
    free(payloadRaw);

  } break;

  case 0x04: // ?? i think im supposed to do error handling here
  {
    set_exit_packet(sendCallback, id, recvPacket->streamId);
  } break;
  default:
    break;
  }

  free(recvPacket);
}

void open_interface(SEND_CALLBACK_TYPE, void *id) {
  size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  struct WispPacket *initPacket = (struct WispPacket *)calloc(1, initSize);
  initPacket->type = CONTINUE_PACKET;
  *(uint32_t *)((char *)&initPacket->payload - 3) = 0x80;

  (sendCallback)(initPacket, initSize, id, false);
}

void close_interface(SEND_CALLBACK_TYPE, void *id) { close_sockets(id); }
