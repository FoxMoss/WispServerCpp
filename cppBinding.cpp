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
#include <vector>
#include <websocketpp/close.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/connection.hpp>
#include <websocketpp/connection_base.hpp>

Server *serverRef;

using ConnectionPtr =
    websocketpp::endpoint<websocketpp::connection<websocketpp::config::asio>,
                          websocketpp::config::asio>::connection_ptr;
void send_callback(void *buffer, size_t size, void *id, bool exit = false) {

  websocketpp::endpoint<websocketpp::connection<websocketpp::config::asio>,
                        websocketpp::config::asio>::connection_ptr con =
      *std::static_pointer_cast<ConnectionPtr>(
          *static_cast<std::shared_ptr<void> *>(id));

  if (con->get_state() == websocketpp::session::state::closing ||
      con->get_state() == websocketpp::session::state::closed) {
    return;
  }
  con->send(buffer, size, websocketpp::frame::opcode::BINARY);
  if (exit) {
  }
}
void on_open(Server *s, websocketpp::connection_hdl hdl) {
  serverRef = s;
  websocketpp::endpoint<websocketpp::connection<websocketpp::config::asio>,
                        websocketpp::config::asio>::connection_ptr ref =
      s->get_con_from_hdl(hdl);

  open_interface(send_callback, ref.get());
}
void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
  auto ref = s->get_con_from_hdl(hdl);

  message_interface(send_callback, msg->get_payload(), ref.get());
}
