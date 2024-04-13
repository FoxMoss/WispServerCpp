#include "interface.hpp"
#include "protocolExtensions.hpp"
#include "wispServer.hpp"
#include "wispValidation.hpp"
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

  if (validatePacket((char *)data, (size_t)msg.length()) == WISP_NULL) {
#ifdef DEBUG
    std::cout << "Bad packet from client\n";
#endif // DEBUG
    return;
  }

  recvPacket->type = *(uint8_t *)data;
  recvPacket->streamId = *(uint32_t *)(data + sizeof(uint8_t));
  memcpy(&recvPacket->payload, data + sizeof(uint8_t) + sizeof(uint32_t),
         payloadLength);

  switch (recvPacket->type) {
  case WISP_CONNECT: // Connect
  {
    struct ConnectPayload *payload =
        (struct ConnectPayload *)malloc(payloadLength + sizeof(ConnectPayload));
    payload->type = *(uint8_t *)(&recvPacket->payload);

    payload->port =
        *(uint16_t *)((char *)&recvPacket->payload + sizeof(uint8_t));
    memcpy(&payload->hostname,
           (char *)((char *)&recvPacket->payload + sizeof(uint8_t) +
                    sizeof(uint16_t)),
           payloadLength - sizeof(uint8_t) - sizeof(uint16_t));

    payload->hostname[payloadLength - 3] = 0;

    open_socket(payload, recvPacket->streamId, sendCallback, id);

    free(payload);
  } break;
  case WISP_DATA: {
    char *payloadRaw = (char *)malloc(payloadLength + 1);

    memcpy(payloadRaw, (char *)(&recvPacket->payload), payloadLength);

    forward_data_packet(recvPacket->streamId, sendCallback, id,
                        (char *)payloadRaw, payloadLength);
    free(payloadRaw);

  } break;

  case WISP_CLOSE:
  default: {
    set_exit_packet(sendCallback, id, recvPacket->streamId);
    break;
  }

  case WISP_INFO: {
    if (matchCompatability >= 2) {
      char *payloadRaw = (char *)malloc(payloadLength + 1);

      memcpy(payloadRaw, (char *)(&recvPacket->payload), payloadLength);

      parse_info_packet(recvPacket->streamId, sendCallback, id, payloadRaw,
                        payloadLength);
      free(payloadRaw);
    } else {
      printf("Warning client %p tried to send an INFO packet on a incompatable "
             "mode.\n",
             id);
    }
    exit(-1);
    break;
  }
  }

  free(recvPacket);
}

void open_interface(SEND_CALLBACK_TYPE, void *id) {
  size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  struct WispPacket *initPacket = (struct WispPacket *)calloc(1, initSize);
  initPacket->type = WISP_CONTINUE;
  *(uint32_t *)((char *)&initPacket->payload - 3) = 0x80;

  sendCallback(initPacket, initSize, id, false);

  if (matchCompatability >= 2) {
    send_info(sendCallback, id);
  }
}

void close_interface(SEND_CALLBACK_TYPE, void *id) { close_sockets(id); }
