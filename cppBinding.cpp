#include "cppBinding.hpp"
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
#include <map>
#include <memory>
#include <string>
#include <sys/types.h>
#include <uWebSockets/WebSocketProtocol.h>
#include <vector>

void send_callback(void *buffer, size_t size, void *id, bool exit = false) {
  uWS::WebSocket<SSL, true, PerSocketData> *ws =
      (uWS::WebSocket<SSL, true, PerSocketData> *)id;

  std::string_view message((char *)buffer, size);
  ws->send(message, uWS::OpCode::BINARY);

  if (exit) {
  }
}
void on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws) {
  open_interface(send_callback, ws);
}
void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws,
                std::string_view message, uWS::OpCode opCode) {

  message_interface(send_callback, std::string(message), ws);
}
