#include "cppBinding.hpp"
#include "interface.hpp"
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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <thread>
#include <uWebSockets/Loop.h>
#include <uWebSockets/WebSocketProtocol.h>
#include <vector>

void send_callback(void *buffer, size_t size, void *id, bool exit = false) {
  uWS::WebSocket<SSL, true, PerSocketData> *ws =
      (uWS::WebSocket<SSL, true, PerSocketData> *)id;

#ifdef DEBUG
  if (validatePacket((char *)buffer, (size_t)size) == WISP_NULL) {
    std::cout << "Bad packet from server\n";
  }
#endif // DEBUG

  ws->getUserData()->loop->defer([=]() {
    std::string_view message((char *)buffer, size);
    if (ws != NULL) {
      ws->send(message, uWS::OpCode::BINARY);

      if (exit) {
        ws->close();
      }
    }
    free(buffer);
  });
}
void on_open(uWS::TemplatedApp<false> *app,
             uWS::WebSocket<SSL, true, PerSocketData> *ws) {
  ws->getUserData()->loop = app->getLoop();
  std::thread(open_interface, send_callback, ws).detach();
}
void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws,
                std::string_view message, uWS::OpCode opCode) {
  if (checkRatelimits(std::string(ws->getRemoteAddressAsText()),
                      (char *)message.data())) {
    ws->close();
    return;
  }
  message_interface(send_callback, std::string(message), ws);
}
