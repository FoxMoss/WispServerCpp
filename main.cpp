#include "anonState.hpp"
#include <cstdlib>
#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <iostream>
#include <jsoncpp/json/json.h>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

typedef server::message_ptr message_ptr;

server anonFoxServer;
AnonState *state;

void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg) {
  if (msg->get_payload() == "stop-listening") {
    s->stop_listening();
    return;
  }
  state->HandleMessage(hdl, msg->get_payload());
}

int main() {
  state = new AnonState(&anonFoxServer);

  try {
    anonFoxServer.set_access_channels(websocketpp::log::alevel::none);
    // anonFoxServer.clear_access_channels(websocketpp::log::alevel::frame_payload);

    anonFoxServer.set_reuse_addr(true);
    anonFoxServer.init_asio();

    anonFoxServer.set_message_handler(
        bind(&on_message, &anonFoxServer, ::_1, ::_2));

    anonFoxServer.listen(6969);

    anonFoxServer.start_accept();

    anonFoxServer.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }

  anonFoxServer.stop();
}
