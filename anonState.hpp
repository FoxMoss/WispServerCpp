#pragma once

#include <jsoncpp/json/config.h>
#include <string>
#include <vector>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> Server;

class AnonState {
public:
  AnonState(Server *cServer);
  ~AnonState() = default;
  void HandleMessage(websocketpp::connection_hdl connectionHdl,
                     std::string data);

private:
  Server *server;
  std::vector<websocketpp::connection_hdl> connections;
  std::mutex connectionsMutex;

  void AddConnection(websocketpp::connection_hdl connectionHdl);
  void RefreshConnections();
  void BroadcastMessage(std::string data, std::string ip);

  void ErrLog(std::string txt);
  void Log(std::string txt);
};
