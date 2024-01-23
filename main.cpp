#include "wispServer.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

server wispServer;

bool validate_func_subprotocol(server *s, std::string *out, std::string accept,
                               websocketpp::connection_hdl hdl) {
  return true;
}

int main(int argv, char *argc[]) {
  if (argv < 2) {
    std::cout << "Port not specified\n";
    return 0;
  }

  std::cout << "Starting on port " << argc[1] << "\n";

  int port = std::stoi(argc[1]);

  try {
    wispServer.set_access_channels(websocketpp::log::alevel::fail);

    wispServer.set_reuse_addr(true);
    wispServer.init_asio();
    wispServer.set_user_agent("WispServer++");
    std::string protcol;

    wispServer.set_validate_handler(bind(
        &validate_func_subprotocol, &wispServer, &protcol, "wisp-v1", ::_1));
    wispServer.set_message_handler(bind(&on_message, &wispServer, ::_1, ::_2));
    wispServer.set_open_handler(bind(&on_open, &wispServer, ::_1));

    wispServer.listen(port);

    wispServer.start_accept();

    wispServer.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }

  wispServer.stop();
}
