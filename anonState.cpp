#include "anonState.hpp"
#include <bits/types/error_t.h>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <string>
#include <vector>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/frame.hpp>

void AnonState::ErrLog(std::string txt) { std::cout << txt << std::endl; }
void AnonState::Log(std::string txt) { std::cout << txt << std::endl; }
AnonState::AnonState(Server *cServer) { server = cServer; }

void AnonState::HandleMessage(websocketpp::connection_hdl connectionHdl,
                              std::string data) {

  Json::Value root;

  Json::Reader reader;
  bool suc = reader.parse(data, root);
  if (!suc) {
    return;
  }

  if (!root["type"].isString() || !root["body"].isString()) {
    ErrLog("Prospect Hacker: " + root.toStyledString());
    return;
  }

  if (root["type"].asString() != "register" &&
      root["type"].asString() != "message") {
    ErrLog("Prospect Hacker: " + root.toStyledString());
    return;
  }

  if (root["type"] == "register") {
    AddConnection(connectionHdl);
  } else if (root["type"] == "message") {
    std::string ip = server->get_con_from_hdl(connectionHdl)
                         ->get_socket()
                         .remote_endpoint()
                         .address()
                         .to_string();

    BroadcastMessage(root["body"].asString(), ip);
  }
}
void AnonState::AddConnection(websocketpp::connection_hdl connectionHdl) {

  RefreshConnections();

  connectionsMutex.lock();
  std::string ip = server->get_con_from_hdl(connectionHdl)
                       ->get_socket()
                       .remote_endpoint()
                       .address()
                       .to_string();
  Log("New connection from " + ip);
  connections.push_back(connectionHdl);

  connectionsMutex.unlock();
}
void AnonState::RefreshConnections() {
  connectionsMutex.lock();

  std::stack<std::vector<websocketpp::connection_hdl>::iterator> deleteQueue;
  for (auto index = connections.begin(); index != connections.end(); index++) {
    websocketpp::connection_hdl connectionHdl =
        *(websocketpp::connection_hdl *)index.base();

    if (connectionHdl.expired()) {
      deleteQueue.push(index);
      ErrLog("Stale connection");
    }
  }
  while (!deleteQueue.empty()) {
    connections.erase(deleteQueue.top());
    deleteQueue.pop();
  }

  connectionsMutex.unlock();
}
void AnonState::BroadcastMessage(std::string data, std::string ip) {
  RefreshConnections();
  connectionsMutex.lock();
  Json::Value root;
  root["type"] = "message";
  root["body"] = data;
  root["ip"] = ip;

  Json::FastWriter fastWriter;
  std::string stringData = fastWriter.write(root);

  Log(stringData);

  for (auto connectionHdl : connections) {
    server->send(connectionHdl, stringData, websocketpp::frame::opcode::TEXT);
  }

  connectionsMutex.unlock();
}
