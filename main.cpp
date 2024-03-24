#include "cppBinding.hpp"
#include "wispServer.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

int port;

int main(int argv, char *argc[]) {
  if (argv < 2) {
    std::cout << "Port not specified\n";
    return 0;
  }

  std::cout << "Starting on port " << argc[1] << "\n";

  port = std::stoi(argc[1]);

  std::vector<std::thread *> threads(std::thread::hardware_concurrency());

  auto app = uWS::App()
                 .ws<PerSocketData>(
                     "*",
                     {
                         .maxPayloadLength = 100 * 1024,
                         .upgrade =
                             [](auto *res, auto *req, auto *context) {
                               res->template upgrade<PerSocketData>(
                                   {}, req->getHeader("sec-websocket-key"), "",
                                   req->getHeader("sec-websocket-extensions"),
                                   context);
                             },
                         .open = on_open,
                         .message = on_message,
                     })
                 .listen(port, [](auto *listen_socket) {
                   if (listen_socket) {
                   }
                 });
  init();

  app.run();
}
