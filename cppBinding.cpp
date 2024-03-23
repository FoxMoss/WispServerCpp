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
#include <mutex>
#include <string>
#include <sys/types.h>
#include <thread>
#include <uWebSockets/Loop.h>
#include <uWebSockets/WebSocketProtocol.h>
#include <vector>

struct Message {
  void *buffer;
  size_t size;
  void *id;
  bool exit = false;
};
std::vector<Message> messageStack;
std::mutex messageLock;

uWS::Loop *mainLoop;

void send_callback(void *buffer, size_t size, void *id, bool exit = false) {
  // void *copyBuffer = malloc(size);
  // memcpy(copyBuffer, buffer, size);
  //
  // messageStack.push_back({copyBuffer, size, id, exit});

  mainLoop->defer([=]() {
    uWS::WebSocket<SSL, true, PerSocketData> *ws =
        (uWS::WebSocket<SSL, true, PerSocketData> *)id;

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
void on_open(uWS::WebSocket<SSL, true, PerSocketData> *ws) {
  std::thread(open_interface, send_callback, ws).detach();
}
void on_message(uWS::WebSocket<SSL, true, PerSocketData> *ws,
                std::string_view message, uWS::OpCode opCode) {
  message_interface(send_callback, std::string(message), ws);
  // std::thread(message_interface, send_callback, std::string(message), ws)
  //     .detach();
}
void init() {
  mainLoop = uWS::Loop::get();
  // struct us_loop_t *loop = (struct us_loop_t *)uWS::Loop::get();
  // struct us_timer_t *delayTimer = us_create_timer(loop, 0, 100);
  //
  // us_timer_set( // stupid but only way to link back into the main thread
  //     delayTimer,
  //     [](struct us_timer_t *t) {
  //       messageLock.lock();
  //       std::map<void *, bool> freeChecker;
  //       while (!messageStack.empty()) {
  //         auto nextMessage = messageStack.begin().base();
  //
  //         uWS::WebSocket<SSL, true, PerSocketData> *ws =
  //             (uWS::WebSocket<SSL, true, PerSocketData> *)nextMessage->id;
  //
  //         if (freeChecker.find(nextMessage->buffer) == freeChecker.end()) {
  //           std::string_view message((char *)nextMessage->buffer,
  //                                    nextMessage->size);
  //           if (ws != NULL) {
  //             ws->send(message, uWS::OpCode::BINARY);
  //
  //             if (nextMessage->exit) {
  //               ws->close();
  //             }
  //           }
  //
  //           freeChecker[nextMessage->buffer] = true;
  //           free(nextMessage->buffer);
  //         }
  //
  //         messageStack.erase(messageStack.begin());
  //       }
  //       messageLock.unlock();
  //     },
  //     1, 1);
}
