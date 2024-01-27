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

std::map<uint32_t, websocketpp::connection_hdl> idMap;
Server *serverRef;
uint32_t nextID = 0;

void send_callback(void *buffer, size_t size, uint32_t id, bool exit = false) {
  if (idMap[id].expired()) {
    return;
  }
  serverRef->send(idMap[id], buffer, size, websocketpp::frame::opcode::BINARY);
  if (exit) {
    serverRef->close(idMap[id], websocketpp::close::status::normal, "");
  }
}
void on_open(Server *s, websocketpp::connection_hdl hdl) {
  serverRef = s;
  idMap[nextID] = hdl;
  open_interface(send_callback, nextID);
  ++nextID;
}
void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
  uint32_t id;
  for (auto member : idMap) {
    if (s->get_con_from_hdl(hdl) == s->get_con_from_hdl(member.second)) {
      id = member.first;
    }
  }

  message_interface(send_callback, msg->get_payload(), id);
}
