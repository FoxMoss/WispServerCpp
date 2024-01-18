#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::server<websocketpp::config::asio> Server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

typedef server::message_ptr message_ptr;

void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg);
void on_open(server *s, websocketpp::connection_hdl hdl);
bool validate_func_subprotocol(server *s, std::string *out, std::string accept,
                               websocketpp::connection_hdl hdl);

#define CONNECT_PACKET 0x01
#define DATA_PACKET 0x02
#define CONTINUE_PACKET 0x03
#define PACKET_SIZE(payload_size)                                              \
  sizeof(uint8_t) + sizeof(uint32_t) + sizeof(char) * payload_size

struct ConnectPayload {
  uint8_t type; // 0x01 == tcp 0x02 == udp
  uint16_t port;
  char hostname[];
};

struct WispPacket {
  uint8_t type;
  uint32_t streamId;
  char payload;
};
