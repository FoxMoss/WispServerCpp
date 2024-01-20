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
#include <vector>
#include <websocketpp/close.hpp>

void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
  size_t payloadLength =
      (size_t)msg->get_payload().length() - sizeof(uint8_t) - sizeof(uint32_t);
  struct WispPacket *recvPacket =
      (struct WispPacket *)malloc((size_t)msg->get_payload().length());
  void *payloadRaw = malloc(payloadLength);

  try {
    const char *data = msg->get_payload().c_str();

    recvPacket->type = *(uint8_t *)data;
    recvPacket->streamId = *(uint32_t *)(data + sizeof(uint8_t));
    std::cout << "packet :" << recvPacket->streamId << "\n";
    memcpy(&recvPacket->payload, data + sizeof(uint8_t) + sizeof(uint32_t),
           payloadLength);

    switch (recvPacket->type) {
    case 0x01: // Connect
    {
      struct ConnectPayload *payload = (struct ConnectPayload *)payloadRaw;
      payload->type = *(uint8_t *)(&recvPacket->payload);

      payload->port =
          *(uint16_t *)((char *)&recvPacket->payload + sizeof(uint8_t));

      memcpy(&payload->hostname,
             (char *)((char *)&recvPacket->payload + sizeof(uint8_t) +
                      sizeof(uint16_t)),
             payloadLength - sizeof(uint8_t) + sizeof(uint16_t));

      payload->hostname[payloadLength - 3] = 0;

      open_socket(payload, recvPacket->streamId, s, hdl);

    } break;
    case 0x02: {
      char *payload = (char *)payloadRaw;
      memcpy(payload, (char *)((char *)&recvPacket->payload), payloadLength);
      forward_data_packet(recvPacket->streamId, s, hdl, payload, payloadLength);
    } break;

    case 0x04: // ?? i think im supposed to do error handling here
    {
      set_exit_packet(s, hdl, recvPacket->streamId);
    } break;
    default:
      break;
    }

  } catch (const std::exception &e) {
    std::cout << "An error occcured \n";

    free(payloadRaw);
    free(recvPacket);
    return;
  }
  free(payloadRaw);
  free(recvPacket);
}

void on_open(server *s, websocketpp::connection_hdl hdl) {
  size_t initSize = PACKET_SIZE((size_t)sizeof(uint32_t));
  struct WispPacket *initPacket = (struct WispPacket *)std::calloc(1, initSize);
  initPacket->type = CONTINUE_PACKET;
  *(uint32_t *)((char *)&initPacket->payload - 3) = 0x80;

  s->send(hdl, initPacket, initSize - 3, websocketpp::frame::opcode::BINARY);
}
