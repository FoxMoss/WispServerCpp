#include "cppBinding.hpp"
#include "wispServer.hpp"
#include "wispValidation.hpp"
#include <bits/getopt_core.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>

int port;

void showHelp(int ret) {

  printf("Usage: wispserver [options] <port>\n");
  printf(" --connect-ratelimit <limit>\t The ammount of connect packets that "
         "can be sent from a single ip per minute.\n");
  printf(" --help\t\t\t Shows this help menu.\n");
  exit(ret);
}

bool isValidInt(char *buf) {
  size_t buflen = strlen(buf);

  for (size_t i = 0; i < buflen; i++) {
    if (buf[i] > '9' || buf[i] < '0') {
      return false;
    }
  }
  return true;
}

int main(int argv, char *argc[]) {
  int option_index = 0;
  const struct option options[] = {
      {"connect-ratelimit", required_argument, NULL, 'c'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  option_index = 0;
  int control;
  while ((control = getopt_long(argv, argc, "", options, &option_index)) !=
         -1) {
    switch (control) {
    case 'c': {
      if (!isValidInt(optarg)) {
        printf("%s: for option --connect-ratelimit value must be an integer.\n",
               argc[0]);
        showHelp(-1);
      }
      maxConnect = std::stoi(optarg);
      break;
    }
    case 'h':
    case '?':
    case ':':
    default:
      showHelp(-1);
    }
  }
  if (argv - optind != 1) {
    printf("%s: one regular argument is needed for port \n", argc[0]);
    showHelp(-1);
  }

  for (int i = optind; i < argv; i++) {
    if (!isValidInt(argc[i])) {
      printf("%s: port must be an integer\n", argc[0]);
      showHelp(-1);
    }
    port = std::stoi(argc[i]);
  }

  std::cout << "Starting on port " << port << "\n";

  std::vector<std::thread *> threads(std::thread::hardware_concurrency());

  std::transform(
      threads.begin(), threads.end(), threads.begin(), [](std::thread * /*t*/) {
        return new std::thread([]() {
          uWS::TemplatedApp<false> app =
              uWS::App()
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
                          .open = [&app](
                                      uWS::WebSocket<SSL, true, PerSocketData>
                                          *ws) { on_open(&app, ws); },
                          .message = on_message,
                      })
                  .listen(port, [](auto *listen_socket) {
                    if (listen_socket) {
                    }
                  });
          app.run();
        });
      });

  std::for_each(threads.begin(), threads.end(),
                [](std::thread *t) { t->join(); });

  printf("Binding closed\n");
}
