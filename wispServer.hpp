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

#define BUFFER_SIZE 128

#define TCP_TYPE 0x01
#define UDP_TYPE 0x02

#define ERROR_UNKNOWN 0x01
#define ERROR_RESET 0x02
#define ERROR_NETWORK_ERROR 0x03
#define ERROR_INVALID_CONNECTION 0x41
#define ERROR_UNREACHABLE 0x42
#define ERROR_CONNECTION_TIMEOUT 0x43
#define ERROR_REFUSED 0x44
#define ERROR_DATA_TIMEOUT 0x47
#define ERROR_BLOCKED 0x48
#define ERROR_THROTTLED 0x49
#define ERROR_CLIENT 0x89

#define CONNECT_PACKET 0x01
#define DATA_PACKET 0x02
#define CONTINUE_PACKET 0x03
#define EXIT_PACKET 0x04
#define PACKET_SIZE(payload_size)                                              \
  sizeof(uint8_t) + sizeof(uint32_t) + (sizeof(char) * (payload_size))

struct ConnectPayload {
  uint8_t type; // 0x01 == tcp 0x02 == udp
  uint16_t port;
  char hostname[];
};

struct WispPacket {
  uint8_t type;
  uint32_t streamId;
  char payload[];
};

struct SocketReference {
  int descriptor;
  uint32_t streamId;
  uint8_t type; // 0x01 == tcp 0x02 == udp
  server *s;
  websocketpp::connection_hdl hdl;
};

void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg);
void on_open(server *s, websocketpp::connection_hdl hdl);
void set_exit_packet(Server *s, websocketpp::connection_hdl hdl,
                     uint32_t streamId = 0, char signal = 0x01);
void set_continue_packet(uint32_t bufferRemaining, Server *s,
                         websocketpp::connection_hdl hdl,
                         uint32_t streamId = 0);
void open_socket(ConnectPayload *payload, uint32_t streamId, Server *s,
                 websocketpp::connection_hdl hdl);
bool validate_func_subprotocol(server *s, std::string *out, std::string accept,
                               websocketpp::connection_hdl hdl);
void forward_data_packet(uint32_t streamId, Server *s,
                         websocketpp::connection_hdl hdl, char *data,
                         size_t length);
void set_data_packet(char *data, size_t size, uint32_t streamId, Server *s,
                     websocketpp::connection_hdl hdl);
void watch_thread(uint32_t streamId);
