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
  struct ConnectPayload *payload =
      (struct ConnectPayload *)malloc(payloadLength);

  try {
    const char *data = msg->get_payload().c_str();

    recvPacket->type = *(uint8_t *)data;
    recvPacket->streamId = *(uint32_t *)(data + sizeof(uint8_t));
    memcpy(&recvPacket->payload, data + sizeof(uint8_t) + sizeof(uint32_t),
           payloadLength);

    switch (recvPacket->type) {
    case 0x01: // Connect
      payload->type = *(uint8_t *)(&recvPacket->payload);

      payload->port = *(uint16_t *)(&recvPacket->payload + sizeof(uint8_t));

      memcpy(
          &payload->hostname,
          (char *)(&recvPacket->payload + sizeof(uint8_t) + sizeof(uint16_t)),
          payloadLength - sizeof(uint8_t) + sizeof(uint16_t));
      break;
    case 0x04: // ?? i think im supposed to do error handling here
      s->close(hdl, websocketpp::close::status::normal, "");
      break;
    }

  } catch (const std::exception &e) {
    std::cout << "An error occcured: " << e.what() << "\n";

    free(payload);
    free(recvPacket);
    return;
  }
  free(payload);
  free(recvPacket);
}

void on_open(server *s, websocketpp::connection_hdl hdl) {
  size_t initSize = PACKET_SIZE(1);
  struct WispPacket *initPacket = (struct WispPacket *)std::calloc(1, initSize);
  initPacket->type = CONTINUE_PACKET;
  initPacket->streamId = 0;
  (&initPacket->payload)[0] = 0;

  s->send(hdl, initPacket, initSize, websocketpp::frame::opcode::BINARY);
}
